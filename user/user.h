#pragma once
#include <string>

class User
{
private:
    std::string m_username;
public:
    User(const std::string& username);
    std::string getUsername() const;
};
