#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "user/user.h"
#include "chat/chat.h"
#include "message/message.h"
#define PORT 8989

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
    
    while (true) {
        addr_size = sizeof(serverStorage);
        newSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&serverStorage), &addr_size);
        
        if (newSocket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_received = recv(newSocket, buffer, 1023, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Null terminate
            std::string request(buffer);
            const char* response;
            ssize_t bytes_sent;

            if (request.find("GET /chats") == 0) 
            {
                response = "Hello client";
                std::cout << newSocket << std::endl;
                bytes_sent = send(newSocket, response, strlen(response), 0);
                std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            } 
            else if (request.find("GET /chat") == 0) 
            {
                response = "Hello client";
                std::cout << newSocket << std::endl;
                bytes_sent = send(newSocket, response, strlen(response), 0);
                std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            } 
            else if (request.find("POST /message") == 0) 
            {
                response = "Hello client";
                std::cout << newSocket << std::endl;
                bytes_sent = send(newSocket, response, strlen(response), 0);
                std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            } 
            else if (request.find("POST /chat") == 0) 
            {
                response = "Hello client";
                std::cout << newSocket << std::endl;
                bytes_sent = send(newSocket, response, strlen(response), 0);
                std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            } 
            else 
            {
                response = "Hello client";
                std::cout << newSocket << std::endl;
                bytes_sent = send(newSocket, response, strlen(response), 0);
                std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            }

        } else {
            std::cerr << "Failed to receive data from client" << std::endl;
        }
        
        close(newSocket);
    }
    
    close(serverSocket);
    return 0;
}
