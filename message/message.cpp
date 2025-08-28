#include "message.h"
#include <iostream>
#include <iomanip>

Message::Message(const User& sender, const std::string& content)
    : m_sender{sender}, m_content{content}, m_timestamp{std::time(nullptr)} {}

User Message::getSender() const { return m_sender; }

std::string Message::getContent() const { return m_content; }

std::time_t Message::getTimestamp() const { return m_timestamp; }

std::ostream& operator<<(std::ostream& os, const Message& message)
{
    std::tm* tm_info = std::localtime(&message.m_timestamp);
    char buffer[26];
    std::strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    os << "[" << buffer << "] " << message.m_sender.getUsername() << ": " << message.m_content;
    return os;
}

