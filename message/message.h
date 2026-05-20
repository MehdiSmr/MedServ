#pragma once
#include <string>
#include <ctime>
#include "../user/user.h"

class Message
{
private:
    User m_sender;
    std::string m_content;
    std::time_t m_timestamp;
public:
    Message(const User& sender, const std::string& content);
    User getSender() const;
    std::string getContent() const;
    std::time_t getTimestamp() const;
    std::string toString() const;
};
