#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat/chat.h"
#include "message/message.h"
#include "threadp/threadp.h"
#include "user/user.h"
#include "utils/utils.h"

#define PORT 8989

static bool startsWith(const std::string& s, const std::string& p) {
    return s.rfind(p, 0) == 0;
}

auto findChatName(const std::vector<std::shared_ptr<Chat>>& chats, const std::string& chatName) {
    return std::find_if(chats.begin(), chats.end(), [&chatName](const std::shared_ptr<Chat>& chat) {
        return chat->getChatName() == chatName;
    });
}

void handleClient(int newSocket, std::vector<std::shared_ptr<Chat>>& chats, std::mutex& chat_mtx, std::mutex& message_mtx) {
    char buffer[1024];

    while (true) {
        std::memset(buffer, 0, sizeof(buffer));
        if (Utils::receiveAll(newSocket, buffer, sizeof(buffer)) == -1) {
            std::cerr << "[servert] receive failed fd=" << newSocket << "\n";
            break;
        }

        std::string request(buffer);
        std::string response("\n");
        std::cout << "[servert] fd=" << newSocket << " req: " << request << "\n";

        if (startsWith(request, "GET /chats")) {
            {
                std::scoped_lock lk(chat_mtx);
                response += "/chats/\n";
                for (const auto& chat : chats) {
                    response += chat->getChatName() + "\n";
                }
            }
        } else if (startsWith(request, "GET /chat/")) {
            const std::string chatName = request.substr(std::strlen("GET /chat/"));
            if (chatName.empty()) {
                response += "Invalid request\n";
            } else {
                std::scoped_lock lk(chat_mtx, message_mtx);
                response += "/chat/" + chatName + "\n";
                const auto index = findChatName(chats, chatName);
                if (index != chats.end()) {
                    response += (*index)->toString();
                } else {
                    response += "Chat not found\n";
                }
            }
        } else if (startsWith(request, "POST /message/")) {
            const size_t msgStart = request.find("/message/");
            const size_t userTag = request.find("/username:", msgStart + 9);
            const size_t openBrace = request.find('{', userTag == std::string::npos ? 0 : userTag + 10);
            const size_t closeBrace = request.rfind('}');

            if (msgStart == std::string::npos || userTag == std::string::npos || openBrace == std::string::npos ||
                closeBrace == std::string::npos || closeBrace <= openBrace) {
                response += "Invalid request\n";
            } else {
                const size_t chatNameStart = msgStart + 9;
                const std::string chatName = request.substr(chatNameStart, userTag - chatNameStart);
                const std::string userName = request.substr(userTag + 10, openBrace - (userTag + 10));
                const std::string content = request.substr(openBrace + 1, closeBrace - openBrace - 1);

                if (chatName.empty() || userName.empty()) {
                    response += "Invalid request\n";
                } else {
                    std::scoped_lock lk(chat_mtx, message_mtx);
                    const auto index = findChatName(chats, chatName);
                    response += "/chat/" + chatName + "\n";
                    if (index != chats.end()) {
                        User sender(userName);
                        auto message = std::make_shared<Message>(sender, content);
                        (*index)->addMessage(message);
                        response += (*index)->toString();
                    } else {
                        response += "Chat not found\n";
                    }
                }
            }
        } else if (startsWith(request, "POST /chat/")) {
            const std::string chatName = request.substr(std::strlen("POST /chat/"));
            if (chatName.empty()) {
                response += "Invalid request\n";
            } else {
                std::scoped_lock lk(chat_mtx);
                const auto index = findChatName(chats, chatName);
                if (index != chats.end()) {
                    response += "Chat already exists\n";
                } else {
                    chats.push_back(std::make_shared<Chat>(chatName));
                    response += "OK\n";
                }
            }
        } else {
            response += "Invalid request\n";
        }

        if (Utils::sendAll(newSocket, response) == -1) {
            std::cerr << "[servert] send failed fd=" << newSocket << "\n";
            break;
        }
    }

    close(newSocket);
    std::cout << "[servert] connection closed fd=" << newSocket << "\n";
}

int main() {
    int serverSocket;
    sockaddr_in serverAddr{};
    sockaddr_storage serverStorage{};
    socklen_t addr_size;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    int yes = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed\n";
        return -1;
    }

    if (listen(serverSocket, 50) != 0) {
        std::cerr << "Listening failed\n";
        return -1;
    }

    unsigned int hc = std::thread::hardware_concurrency();
    if (hc == 0) {
        hc = 2;
    }
    ThreadPool pool(hc * 2);

    std::vector<std::shared_ptr<Chat>> chats;
    std::mutex chat_mtx;
    std::mutex message_mtx;

    std::cout << "[servert] listening on port " << PORT << "\n";

    while (true) {
        addr_size = sizeof(serverStorage);
        const int newSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&serverStorage), &addr_size);
        if (newSocket < 0) {
            std::cerr << "[servert] accept failed\n";
            continue;
        }

        std::cout << "[servert] new connection fd=" << newSocket << "\n";
        pool.enqueue([newSocket, &chats, &chat_mtx, &message_mtx] {
            handleClient(newSocket, chats, chat_mtx, message_mtx);
        });
    }

    close(serverSocket);
    return 0;
}
