#ifndef USER_H
#define USER_H

#include <string>
#include <unordered_map>

struct UserData {
    int id;
    std::string firstname;
    std::string lastname;
    std::string nickname;
    std::string password;
    std::string hisChat;
};

#endif