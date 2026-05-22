#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/ioctl.h>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "utils/utils.h"
#define PORT 8989

const int detectConsoleLines() {
    winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
       return static_cast<int>(w.ws_row/2);
    }
   return 50; // fallback
};

namespace Settings
{
    const int g_consolelines(detectConsoleLines());
}

int getChatRequest(const char* chat, const char* username, int client_fd)
{
    const std::string request = "GET /chat/" + std::string(chat); 
    const int status = Utils::sendAll(client_fd, request);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int getChatsRequest(const char* username, int client_fd)
{
    const std::string request = "GET /chats"; 
    const int status = Utils::sendAll(client_fd, request);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int createChatRequest(const char* chat, const char* username, int client_fd)
{
    const std::string request = "POST /chat/" + std::string(chat); 
    const int status = Utils::sendAll(client_fd, request);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int sendMessageRequest(const char* chat, const char* message, const char* username, int client_fd)
{
    const std::string request = "POST /message/" + std::string(chat) + "{" + std::string(message) + "}"; 
    const int status = Utils::sendAll(client_fd, request);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int main() {
    int status, client_fd;
    struct sockaddr_in serverAddr;
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
  
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }
    
    if ((status = connect(client_fd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr))) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    char username[1024];
    std::vector<std::string> chats;
    
    for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
    for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
    std::cout << "Enter username: ";
    std::cin >> username;
    
    char rbuffer[1024];
    char wbuffer[1024]; 
    while (1) {
        getChatsRequest(username, client_fd);
        memset(rbuffer, 0, sizeof(rbuffer));
        chats.clear();
        if (Utils::receiveAll(client_fd, rbuffer, sizeof(rbuffer)) != -1) {
            std::string response(rbuffer);
            if (response == "\n") {
                for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                std::cout << "No chats available" << std::endl;
                for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
            } else {
                std::stringstream iss(response);
                std::string chatName;
                while (std::getline(iss, chatName)) {
                    if (!chatName.empty()) {
                        chats.push_back(chatName);
                    }
                }
                for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                std::cout << "Available chats:" << std::endl;
                for (const auto& chat : chats) {
                    std::cout << "- " << chat << std::endl;
                }
                for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                std::cout << "Enter chat name to join: ";
                std::cin >> chatName;
                getChatRequest(chatName.c_str(), username, client_fd);
                memset(rbuffer, 0, sizeof(rbuffer)); 
                if (Utils::receiveAll(client_fd, rbuffer, sizeof(rbuffer)) != -1) {
                    for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                    std::cout << rbuffer << std::endl;
                    for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                    std::cout << "Welcome to " << chatName << " chat! If you want to leave the chat, type '/exit'" << std::endl; 
                    while(1) {
                        std::cout << "Enter message: ";
                        memset(wbuffer, 0, sizeof(wbuffer));
                        std::cin>>std::ws;
                        std::cin.getline(wbuffer, sizeof(wbuffer));
                        if (std::string(wbuffer) == "/exit") {
                            break;
                        }
                        sendMessageRequest(chatName.c_str(), wbuffer, username, client_fd);
                        memset(rbuffer, 0, sizeof(rbuffer));
                        if (Utils::receiveAll(client_fd, rbuffer, sizeof(rbuffer)) != -1) {
                            for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                            std::cout << rbuffer << std::endl;
                            for (int i = 0; i < Settings::g_consolelines; ++i) {std::cout << std::endl;}
                        } else {
                            // erreeuurr
                        }
                    }
                } else {
                   // erreeeur 
                }
            } 
        }else {
            // erreeuurr
        }
    }
    
    close(client_fd);
    return 0;
}
