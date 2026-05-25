#include <algorithm>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "chat/chat.h"
#include "message/message.h"
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

void add_to_pfds(std::vector<pollfd>& pfds, int newfd) {
    pfds.push_back(pollfd{newfd, POLLIN, 0});
}

void del_from_pfds(std::vector<pollfd>& pfds, size_t i) {
    pfds.erase(pfds.begin() + static_cast<long>(i));
}

void handle_new_connection(int serverSocket, std::vector<pollfd>& pfds) {
    sockaddr_storage remoteaddr{};
    socklen_t addr_size = sizeof(remoteaddr);

    const int newSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&remoteaddr), &addr_size);
    if (newSocket < 0) {
        std::cerr << "[serverp] accept failed\n";
        return;
    }

    add_to_pfds(pfds, newSocket);
    std::cout << "[serverp] new connection fd=" << newSocket << "\n";
}

bool sendToClientOrDrop(std::vector<pollfd>& pfds, size_t i, const std::string& response, const char* context) {
    const int fd = pfds[i].fd;
    if (Utils::sendAll(fd, response) == 0) {
        return true;
    }

    std::cerr << "[serverp] send failed (" << context << ") fd=" << fd << ", dropping client\n";
    close(fd);
    del_from_pfds(pfds, i);
    return false;
}

bool broadcastOrDrop(int serverSocket, int senderFd, std::vector<pollfd>& pfds, const std::string& response) {
    bool senderAlive = true;

    for (size_t i = 0; i < pfds.size();) {
        const int fd = pfds[i].fd;
        if (fd == serverSocket) {
            ++i;
            continue;
        }

        if (Utils::sendAll(fd, response) == 0) {
            ++i;
            continue;
        }

        std::cerr << "[serverp] broadcast send failed fd=" << fd << ", dropping client\n";
        if (fd == senderFd) {
            senderAlive = false;
        }
        close(fd);
        del_from_pfds(pfds, i);
    }

    return senderAlive;
}

bool handleClient(int serverSocket, size_t i, std::vector<pollfd>& pfds, std::vector<std::shared_ptr<Chat>>& chats) {
    const int newSocket = pfds[i].fd;

    char buffer[1024]{};
    if (Utils::receiveAll(newSocket, buffer, sizeof(buffer)) == -1) {
        std::cerr << "[serverp] receive failed fd=" << newSocket << ", dropping client\n";
        close(newSocket);
        del_from_pfds(pfds, i);
        return false;
    }

    std::string request(buffer);
    std::string response("\n");
    std::cout << "[serverp] fd=" << newSocket << " req: " << request << "\n";

    if (startsWith(request, "GET /chats")) {
        response += "/chats/\n";
        for (const auto& chat : chats) {
            response += chat->getChatName() + "\n";
        }
        return sendToClientOrDrop(pfds, i, response, "GET /chats");
    }

    if (startsWith(request, "GET /chat/")) {
        const std::string chatName = request.substr(std::strlen("GET /chat/"));
        if (chatName.empty()) {
            response += "Invalid request\n";
            return sendToClientOrDrop(pfds, i, response, "GET /chat invalid");
        }

        response += "/chat/" + chatName + "\n";
        const auto index = findChatName(chats, chatName);
        if (index != chats.end()) {
            response += (*index)->toString();
        } else {
            response += "Chat not found\n";
        }

        return sendToClientOrDrop(pfds, i, response, "GET /chat");
    }

    if (startsWith(request, "POST /message/")) {
        const size_t msgStart = request.find("/message/");
        const size_t userTag = request.find("/username:", msgStart + 9);
        const size_t openBrace = request.find('{', userTag == std::string::npos ? 0 : userTag + 10);
        const size_t closeBrace = request.rfind('}');

        if (msgStart == std::string::npos || userTag == std::string::npos || openBrace == std::string::npos ||
            closeBrace == std::string::npos || closeBrace <= openBrace) {
            response += "Invalid request\n";
            return sendToClientOrDrop(pfds, i, response, "POST /message parse");
        }

        const size_t chatNameStart = msgStart + 9;
        const std::string chatName = request.substr(chatNameStart, userTag - chatNameStart);
        const std::string userName = request.substr(userTag + 10, openBrace - (userTag + 10));
        const std::string content = request.substr(openBrace + 1, closeBrace - openBrace - 1);

        if (chatName.empty() || userName.empty()) {
            response += "Invalid request\n";
            return sendToClientOrDrop(pfds, i, response, "POST /message empty fields");
        }

        const auto index = findChatName(chats, chatName);
        if (index == chats.end()) {
            response += "/chat/" + chatName + "\nChat not found\n";
            return sendToClientOrDrop(pfds, i, response, "POST /message chat missing");
        }

        User sender(userName);
        auto message = std::make_shared<Message>(sender, content);
        (*index)->addMessage(message);

        response += "/chat/" + chatName + "\n" + (*index)->toString() + "\n";
        const bool senderAlive = broadcastOrDrop(serverSocket, newSocket, pfds, response);
        return senderAlive;
    }

    if (startsWith(request, "POST /chat/")) {
        const std::string chatName = request.substr(std::strlen("POST /chat/"));
        if (chatName.empty()) {
            response += "Invalid request\n";
            return sendToClientOrDrop(pfds, i, response, "POST /chat invalid");
        }

        const auto index = findChatName(chats, chatName);
        if (index != chats.end()) {
            response += "Chat already exists\n";
            return sendToClientOrDrop(pfds, i, response, "POST /chat exists");
        }

        chats.push_back(std::make_shared<Chat>(chatName));

        response += "/chats/\n";
        for (const auto& chat : chats) {
            response += chat->getChatName() + "\n";
        }

        const bool senderAlive = broadcastOrDrop(serverSocket, newSocket, pfds, response);
        return senderAlive;
    }

    response += "Invalid request\n";
    return sendToClientOrDrop(pfds, i, response, "unknown request");
}

void process_connections(int serverSocket, std::vector<pollfd>& pfds, std::vector<std::shared_ptr<Chat>>& chats) {
    for (size_t i = 0; i < pfds.size();) {
        const short ev = pfds[i].revents;
        if ((ev & (POLLIN | POLLHUP | POLLERR)) == 0) {
            ++i;
            continue;
        }

        if (pfds[i].fd == serverSocket) {
            handle_new_connection(serverSocket, pfds);
            ++i;
            continue;
        }

        if (ev & (POLLHUP | POLLERR)) {
            std::cout << "[serverp] fd=" << pfds[i].fd << " hung up\n";
            close(pfds[i].fd);
            del_from_pfds(pfds, i);
            continue;
        }

        const bool stillAlive = handleClient(serverSocket, i, pfds, chats);
        if (stillAlive) {
            ++i;
        }
    }
}

int main() {
    int serverSocket;
    std::vector<pollfd> pfds;
    sockaddr_in serverAddr{};

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

    pfds.push_back(pollfd{serverSocket, POLLIN, 0});
    std::vector<std::shared_ptr<Chat>> chats;

    std::cout << "[serverp] listening on port " << PORT << "\n";

    for (;;) {
        const int poll_count = poll(pfds.data(), pfds.size(), -1);
        if (poll_count == -1) {
            std::cerr << "[serverp] poll failed\n";
            return -1;
        }

        process_connections(serverSocket, pfds, chats);
    }

    close(serverSocket);
    return 0;
}
