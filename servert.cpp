#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <mutex>
#include "user/user.h"
#include "chat/chat.h"
#include "message/message.h"
#include "utils/utils.h"
#include "threadp/threadp.h"
#define PORT 8989

auto findChatName(const std::vector<std::shared_ptr<Chat>>& chats, const std::string& chatName) {
    auto it = std::find_if(chats.begin(), chats.end(), [&chatName](const std::shared_ptr<Chat>& chat) {
        return chat->getChatName() == chatName;
    });
    return it; // Return iterator to the found chat or chats.end() if not found
}

void handleClient(int newSocket, std::vector<std::shared_ptr<Chat>>& chats, std::mutex& chat_mtx, std::mutex& message_mtx)
{
    char buffer[1024];
    while (1) { 
        memset(buffer, 0, sizeof(buffer));
        if (Utils::receiveAll(newSocket, buffer, sizeof(buffer) - 1) != -1) {
            std::string request(buffer);
            std::string response;
            ssize_t success {0};

            response = "\n";
            if (request.find("GET /chats") == 0){
                { 
                    std::scoped_lock lk(chat_mtx); 
                    if (!chats.empty()) {
                        for (const auto& chat : chats) {
                            response += chat->getChatName() + "\n";
                        }
                    }
                }
                success = Utils::sendAll(newSocket, response);
            } 
            else if (request.find("GET /chat") == 0) 
            {
                std::string chatName = request.substr(request.find("/chat") + 6);// Extract chat name from request 
                { 
                    std::scoped_lock lk(chat_mtx, message_mtx); // Lock both mutexes to ensure thread safety when accessing chats and messages 
                    auto index = findChatName(chats, chatName); 
                    if (index != chats.end()) {
                        response += (*index)->toString();
                    } else {
                        response += "Chat not found\n";
                    }
                }
                success = Utils::sendAll(newSocket, response);
            } 
            else if (request.find("POST /message") == 0) 
            {
                size_t start = request.find("/message/") + 9;
                size_t open  = request.find("/username:", start);
                std::string chatName = request.substr(start, open - start);
                { 
                    std::scoped_lock lk(chat_mtx); // Lock the mutex before accessing the chats vector 
                    auto index = findChatName(chats, chatName);
                    std::string userName = request.substr(open + 10, request.find("{") - (open + 10));
                    if (index != chats.end()) {
                        User sender(userName);
                        size_t start = request.find("{") + 1; // points to 'G'
                        size_t open  = request.find("}");
                        std::string content  = request.substr(start, open - start);
                        {
                            std::scoped_lock lk2(message_mtx); // Lock the mutex before modifying the messages vector 
                            auto message = std::make_shared<Message>(sender, content);
                            (*index)->addMessage(message);
                            response += (*index)->toString();
                        }
                    } else {
                        response += "Chat not found\n";
                    }
                } 
                success = Utils::sendAll(newSocket, response);
            } 
            else if (request.find("POST /chat") == 0) 
            {
                std::string chatName = request.substr(request.find("/chat") + 6);
                {
                    std::scoped_lock lk(chat_mtx); // Lock the mutex before modifying the chats vector 
                    auto index = findChatName(chats, chatName);
                    if (index != chats.end()) {
                        response += "Chat already exists\n";
                    } else {
                        chats.push_back(std::make_shared<Chat>(chatName));
                        response += "OK\n";
                    }
                }
                success = Utils::sendAll(newSocket, response);
            } 
            else 
            {
                response += "Invalid request\n";
                success = Utils::sendAll(newSocket, response);
            }
            if (success == -1) {
                std::cerr << "Failed to send response to client" << std::endl;
                break;
            }
        } else {
            std::cerr << "Failed to receive data from client" << std::endl;
            break;
        }
    }
    close(newSocket);
}

int main() {
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return -1;
    }
    
    if (listen(serverSocket, 50) != 0) {
        std::cerr << "listening failed" << std::endl;
        return -1;
    }
    ThreadPool pool(std::thread::hardware_concurrency() * 2); // Create a thread pool with twice the number of hardware threads
    std::vector<std::shared_ptr<Chat>> chats; 
    std::mutex chat_mtx; // Mutex to protect access to the chats vector
    std::mutex message_mtx; // Mutex to protect access to the messages vector
    while (1) {
        addr_size = sizeof(serverStorage);
        
        newSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&serverStorage), &addr_size);

        if (newSocket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        pool.enqueue([newSocket, &chats, &chat_mtx, &message_mtx]{handleClient(newSocket, chats, chat_mtx, message_mtx);}); 
    }
    
    close(serverSocket);
    return 0;
}
