#ifndef GLOBALS_H
#define GLOBALS_H

#include "libpq-fe.h"
#include <unordered_map>
#include <string>

// Global database connection object for PostgreSQL
extern PGconn* db;

// Map to track connected users (user ID -> connection status)
extern std::unordered_map<int, bool> connectedUsers;

// Variable to store the latest user ID
extern int latest_user_id;

#endif