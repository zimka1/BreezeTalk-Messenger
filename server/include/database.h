#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include "libpq-fe.h"
#include <cstring>

// Initialize the database
void initDatabase();

// Get the last user ID from the database
int getLastUserId(PGconn* conn);

// Print the users table
void printUsersTable(PGconn* conn);

#endif