#ifndef GLOBALS_H
#define GLOBALS_H

#include <sqlite3.h>
#include <unordered_map>
#include <string>

extern sqlite3* db;
extern std::unordered_map<int, bool> connectedUsers;
extern int latest_user_id;

#endif