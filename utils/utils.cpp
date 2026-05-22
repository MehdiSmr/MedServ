#include "utils.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <sys/ioctl.h>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

int Utils::sendAll(int s, const std::string& data) {
    int total = 0; // how many bytes we've sent
    char buffer[1024];
    uint32_t netLen = htonl(static_cast<uint32_t>(data.size()));
    std::memcpy(buffer, &netLen, 4);
    std::memcpy(buffer + 4, data.c_str(), data.size());
    size_t len = 4 + data.size(); // total length of the data 
    size_t bytesleft = len; // how many we have left to send
    int n;
    while (total < len) {
        n = send(s, buffer + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

int Utils::receiveAll(int s, char* buf, size_t len) {
    if (len < 4) {
        std::cerr << "Buffer too small to read header" << std::endl;
        return -1;
    } 
    int n, total = 0; // how many bytes we've received
    while (total < 4) { // first read the header (4 bytes)
        n = recv(s, buf + total, 4 - total, 0);
        if (n <= 0) {
            if (n < 0) {
                perror("recv error");
                std::cerr << "errno: " << errno << std::endl;
            } else {
                std::cout << "Connection closed by server" << std::endl;
            }
            return -1;
        }
        total += n;
    }
    uint32_t netLen;
    std::memcpy(&netLen, buf, 4);
    uint32_t dataSize = ntohl(netLen) + 4;
    if (dataSize > len) {
        std::cerr << "Buffer too small for incoming data" << std::endl;
        return -1;
    }
    while (total < dataSize && n > 0) {
        n = recv(s, buf + total, dataSize - total, 0);
        if (n <= 0) { break; }
        total += n;
    }
    if (n < 0) {
        perror("recv error");
        std::cerr << "errno: " << errno << std::endl;
        return -1;
    } else if (total == 0) {
        std::cout << "Connection closed by server" << std::endl;
        return -1;
    }
    memmove(buf, buf + 4, total - 4); // Shift data to the beginning of the buffer
    buf[total - 4] = '\0'; // Null terminate the buffer
    return 0; // return 0 on success
}
