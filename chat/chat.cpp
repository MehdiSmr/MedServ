#include "chat.h"

Chat::Chat(const std::string& chatName) 
    : m_chatName{chatName} {}

void Chat::addMessage(const std::shared_ptr<Message>& message) {m_messages.push_back(message);}

std::string Chat::getChatName() const{ return m_chatName;}

std::string Chat::toString() const{ 
    std::string result;
    for (const auto& message : m_messages) {
        result += message->toString() + "\n";
    }
    return result;
}

const std::vector<std::shared_ptr<Message> >& Chat::getMessages() const{ return m_messages;}
