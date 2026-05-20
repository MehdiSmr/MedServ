#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../message/message.h"
#include "../user/user.h"

class Chat
{
private:
    std::string m_chatName;
    std::vector<std::shared_ptr<Message> > m_messages;
public:
    Chat(const std::string& chatName);
    void addMessage(const std::shared_ptr<Message>& message);
    std::string getChatName() const;
    std::string toString() const;
    const std::vector<std::shared_ptr<Message> >& getMessages() const;
};
