#include <iostream>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define PORT 8989

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
    
    status = send(client_fd, "hello", 5, 0);
    std::cout << "HTTP request sent, bytes sent: " << status << std::endl;
    
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    std::cout << client_fd << std::endl;
    
    ssize_t bytes_read = recv(client_fd, buffer, 1023, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';  // Null terminate
        std::string response(buffer);
        std::cout << "Received response: '" << response << "'" << std::endl;
    } else if (bytes_read == 0) {
        std::cout << "Connection closed by server" << std::endl;
    } else {
        perror("recv error");
        std::cout << "errno: " << errno << std::endl;
    }
    
    close(client_fd);
    return 0;
}
