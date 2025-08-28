#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define PORT 8989

int getChatRequest(const char* chat, int client_fd)
{
    std::string request = "GET /chat/" + std::string(chat); 
    const int status = send(client_fd, request.c_str(), strlen(request.c_str()), 0);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int getChatsRequest(const char* username, int client_fd)
{
    const char* request = "GET /chat"; 
    const int status = send(client_fd, request, strlen(request), 0);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int createChatRequest(const char* chat, const char* username, int client_fd)
{
    std::string request = "POST /chat/" + std::string(chat); 
    const int status = send(client_fd, request.c_str(), strlen(request.c_str()), 0);
    if (status < 0) {
        std::cerr << "Sending Request error" << std::endl;
        return -1;
    }
    return 0;
};

int sendMessageRequest(const char* message, const char* username, int client_fd)
{
    std::string request = "GET /message/" + std::string(message); 
    const int status = send(client_fd, request.c_str(), strlen(request.c_str()), 0);
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
    bool session{true};
    bool inChat{false};
    
    std::cout << "Enter username: ";
    std::cin >> username;

    while (session) { 
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        if (!inChat) {
            getChatsRequest(username, client_fd); // est ce qu'on attend la prochaine iteration ou est ce j'enchaine tout
        } else {
        }


        ssize_t bytes_read = recv(client_fd, buffer, 1023, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';  // Null terminate
            std::string response(buffer);
            std::cout << "Received response: '" << response << "'" << std::endl;
        } else if (bytes_read == 0) {
            std::cout << "Connection closed by server" << std::endl;
            session = false;
        } else {
            perror("recv error");
            std::cout << "errno: " << errno << std::endl;
            session = false;
        }
    }
    
    close(client_fd);
    return 0;
}
