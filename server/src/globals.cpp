#include "../include/globals.h"

sqlite3* db = nullptr;
std::unordered_map<int, bool> connectedUsers;
int latest_user_id = 0;