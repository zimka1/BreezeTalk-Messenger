#include "../include/globals.h"

PGconn* db = nullptr; // PostgreSQL database connection
std::unordered_map<int, bool> connectedUsers; // Map to track connected users
int latest_user_id = 0; // Latest user ID