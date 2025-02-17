#include "../include/database.h"
#include "../include/globals.h"



void initDatabase() {
    if (sqlite3_open("chat.db", &db)) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    const char* createUsersTable = "CREATE TABLE IF NOT EXISTS users ("
                                   "id INTEGER PRIMARY KEY,"
                                   "firstname TEXT NOT NULL,"
                                   "lastname TEXT NOT NULL,"
                                   "nickname TEXT NOT NULL,"
                                   "password TEXT NOT NULL);";

    const char* createMessagesTable = "CREATE TABLE IF NOT EXISTS messages ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "user_from INTEGER,"
                                      "user_to INTEGER,"
                                      "hasBeenRead INTEGER,"
                                      "message TEXT NOT NULL,"
                                      "FOREIGN KEY(user_from) REFERENCES users(id),"
                                      "FOREIGN KEY(user_to) REFERENCES users(id));";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, createUsersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(1);
    }

    if (sqlite3_exec(db, createMessagesTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating messages table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(1);
    }
}

int getLastUserId(sqlite3* db) {
    const char* getMaxIdSQL = "SELECT MAX(id) FROM users";
    sqlite3_stmt* stmt;
    int lastId = 0;

    if (sqlite3_prepare_v2(db, getMaxIdSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            lastId = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
    }

    return lastId;
}

void printUsersTable() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        int cols = sqlite3_column_count(stmt);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int col = 0; col < cols; col++) {
                const char* colName = sqlite3_column_name(stmt, col);
                const char* colText = (const char*)sqlite3_column_text(stmt, col);
                std::cout << colName << ": " << (colText ? colText : "NULL") << " | ";
            }
            std::cout << std::endl;
        }

        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
    }
}