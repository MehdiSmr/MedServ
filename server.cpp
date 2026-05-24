#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include "user/user.h"
#include "chat/chat.h"
#include "message/message.h"
#include "utils/utils.h"
#define PORT 8989

auto findChatName(const std::vector<std::shared_ptr<Chat>>& chats, const std::string& chatName) {
    auto it = std::find_if(chats.begin(), chats.end(), [&chatName](const std::shared_ptr<Chat>& chat) {
        return chat->getChatName() == chatName;
    });
    return it; // Return iterator to the found chat or chats.end() if not found
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

    std::vector<std::shared_ptr<Chat>> chats; 
    char buffer[1024];
    while (1) {
        addr_size = sizeof(serverStorage);
        
        newSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&serverStorage), &addr_size);

        if (newSocket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        while (1) { 
            memset(buffer, 0, sizeof(buffer));
            
            if (Utils::receiveAll(newSocket, buffer, 1023) != -1) {
                std::string request(buffer);
                std::string response;
                ssize_t bytes_sent;

                response = "\n";
                if (request.find("GET /chats") == 0){ 
                    if (!chats.empty()) {
                        for (const auto& chat : chats) {
                            response += chat->getChatName() + "\n";
                        }
                    }
                    bytes_sent = Utils::sendAll(newSocket, response);
                } 
                else if (request.find("GET /chat") == 0) 
                {
                    std::string chatName = request.substr(request.find("/chat") + 6);// Extract chat name from request 
                    auto index = findChatName(chats, chatName); 
                    if (index != chats.end()) {
                        response += (*index)->toString();
                    } else {
                        response += "Chat not found\n";
                    }
                    bytes_sent = Utils::sendAll(newSocket, response);
                } 
                else if (request.find("POST /message") == 0) 
                {
                    size_t start = request.find("/message/") + 9;
                    size_t open  = request.find("/username:", start);
                    std::string chatName = request.substr(start, open - start);
                    auto index = findChatName(chats, chatName);
                    std::string userName = request.substr(open + 10, request.find("{") - (open + 10));
                    if (index != chats.end()) {
                        User sender(userName);
                        size_t start = request.find("{") + 1; // points to 'G'
                        size_t open  = request.find("}");
                        std::string content  = request.substr(start, open - start);
                        auto message = std::make_shared<Message>(sender, content);
                        (*index)->addMessage(message);
                        response += (*index)->toString();
                        bytes_sent = Utils::sendAll(newSocket, response);
                    }
                } 
                else if (request.find("POST /chat") == 0) 
                {
                    std::string chatName = request.substr(request.find("/chat") + 6);
                    auto index = findChatName(chats, chatName);
                    if (index != chats.end()) {
                        response += "Chat already exists\n";
                    } else {
                        chats.push_back(std::make_shared<Chat>(chatName));
                        response += "OK\n";
                    }
                    bytes_sent = Utils::sendAll(newSocket, response);
                } 
                else 
                {
                    response += "Invalid request\n";
                    bytes_sent = Utils::sendAll(newSocket, response);
                }

            } else {
                std::cerr << "Failed to receive data from client" << std::endl;
                break; // Exit the loop if the client has disconnected or an error occurred 
            }
        } 
        close(newSocket);
    }
    
    close(serverSocket);
    return 0;
}
