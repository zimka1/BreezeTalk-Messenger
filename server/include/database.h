#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <iostream>

void initDatabase();
int getLastUserId(sqlite3* db);
void printUsersTable();

#endif