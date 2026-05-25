#include "utils.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

int Utils::sendAll(int s, const std::string& data) {
    const uint32_t payloadLen = static_cast<uint32_t>(data.size());
    const uint32_t netLen = htonl(payloadLen);

    size_t sent = 0;
    while (sent < sizeof(netLen)) {
        const ssize_t n = send(s,
                               reinterpret_cast<const char*>(&netLen) + sent,
                               sizeof(netLen) - sent,
                               0);
        if (n <= 0) {
            return -1;
        }
        sent += static_cast<size_t>(n);
    }

    sent = 0;
    while (sent < data.size()) {
        const ssize_t n = send(s, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) {
            return -1;
        }
        sent += static_cast<size_t>(n);
    }

    return 0;
}

int Utils::receiveAll(int s, char* buf, size_t len) {
    if (len < 1) {
        std::cerr << "Buffer too small" << std::endl;
        return -1;
    }

    uint32_t netLen = 0;
    size_t received = 0;
    while (received < sizeof(netLen)) {
        const ssize_t n = recv(s,
                               reinterpret_cast<char*>(&netLen) + received,
                               sizeof(netLen) - received,
                               0);
        if (n <= 0) {
            if (n < 0) {
                perror("recv error");
                std::cerr << "errno: " << errno << std::endl;
            } else {
                std::cout << "Connection closed by peer" << std::endl;
            }
            return -1;
        }
        received += static_cast<size_t>(n);
    }

    const uint32_t payloadLen = ntohl(netLen);
    if (payloadLen >= len) {
        std::cerr << "Incoming frame too large for buffer" << std::endl;
        return -1;
    }

    received = 0;
    while (received < payloadLen) {
        const ssize_t n = recv(s, buf + received, payloadLen - received, 0);
        if (n <= 0) {
            if (n < 0) {
                perror("recv error");
                std::cerr << "errno: " << errno << std::endl;
            } else {
                std::cout << "Connection closed by peer" << std::endl;
            }
            return -1;
        }
        received += static_cast<size_t>(n);
    }

    buf[payloadLen] = '\0';
    return 0;
}
