#pragma once
#include <string>


namespace Utils
{
    int sendAll(int s, const std::string& data);
    int receiveAll(int s, char* buf, size_t len);
}
