#include "user.h"

User::User(const std::string& username) : m_username{username} {}

std::string User::getUsername() const { return m_username; }
