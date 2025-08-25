#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>  // for strlen
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
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
        std::cout << "Connection accepted" << std::endl;
        
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
            std::cout << "Received request: " << request << std::endl;
            
            const char* response = "Hello client";
            std::cout << newSocket << std::endl;
            ssize_t bytes_sent = send(newSocket, response, strlen(response), 0);
            std::cout << "HTTP response sent, bytes sent: " << bytes_sent << std::endl;
            
        } else {
            std::cerr << "Failed to receive data from client" << std::endl;
        }
        
        close(newSocket);
        std::cout << "Connection closed" << std::endl;
    }
    
    close(serverSocket);
    std::cout << "Server shutdown" << std::endl;
    return 0;
}
