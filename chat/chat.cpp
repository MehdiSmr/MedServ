#include "chat.h"

Chat::Chat(const std::string& chatName) 
    : m_chatName{chatName} {}

void addMessage(const std::shared_ptr<Message>& message) {m_messages.push_back(message);}

std::string getChatName() const{ return m_chatName;}

const std::vector<std::shared_ptr<Message> >& getMessages() const{ return m_messages;}
