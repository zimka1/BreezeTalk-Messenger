#include "../include/message.h"
#include "../include/globals.h"


using namespace std;

void saveMessage(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
//    cout << parsed_data["user_from"] << " " << parsed_data["user_to"] << " " << endl;
    int user_from = parsed_data["user_from"];
    int user_to = parsed_data["user_to"];
    int hasBeenRead = parsed_data["hasBeenRead"];
    string message = parsed_data["message"];

    const char* insertMessageSQL = "INSERT INTO messages (user_from, user_to, hasBeenRead, message) VALUES (?, ?, ?, ?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertMessageSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, hasBeenRead);
        sqlite3_bind_text(stmt, 4, message.c_str(), -1, SQLITE_STATIC);

        json acknowledgement {
                {"command", "sentAcknowledgement"},
                {"user_from", user_from},
                {"user_to", user_to},
                {"message", message}
        };

        if (user_from != udata->id) {
            ws->publish("user" + to_string(user_from), acknowledgement.dump());
        } else {
            ws->send(acknowledgement.dump(), uWS::OpCode::TEXT);
        }


        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cerr << "Error inserting message: " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing insert statement: " << sqlite3_errmsg(db) << endl;
    }
}

void markMessagesAsRead(int user_from, int user_to) {
    const char* updateMessagesSQL = "UPDATE messages SET hasBeenRead = 1 WHERE (user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?)";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, updateMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cerr << "Error updating messages: " << sqlite3_errmsg(db) << endl;
        }

        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing statement: " << sqlite3_errmsg(db) << endl;
    }
}

json getAllMessages(int user_from, int user_to) {
    const char* getMessagesSQL = "SELECT user_from, user_to, message FROM messages WHERE (user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?)";
    sqlite3_stmt* stmt;
    json messages = json::array();

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int from = sqlite3_column_int(stmt, 0);
            int to = sqlite3_column_int(stmt, 1);
            const char* messageText = (const char*)sqlite3_column_text(stmt, 2);
//            cout << from << ' ' << to << ' ' << messageText << endl;
            messages.push_back({{"from", from}, {"to", to}, {"message", messageText}});
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return messages;
}

json getOnlyReadMessages(int user_from, int user_to) {
    const char* getMessagesSQL = "SELECT user_from, user_to, hasBeenRead, message FROM messages WHERE ((user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?)) AND hasBeenRead = 1";
    sqlite3_stmt* stmt;
    json messages = json::array();

//    cout << "RRRRRRREADDDDDDDDDD:" << endl;

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int from = sqlite3_column_int(stmt, 0);
            int to = sqlite3_column_int(stmt, 1);
            const char* messageText = (const char*)sqlite3_column_text(stmt, 3);
//            cout << from << ' ' << to << ' ' << messageText << endl;
            messages.push_back({{"from", from}, {"to", to}, {"message", messageText}});
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return messages;
}

json getOnlyUnreadMessages(int user_from, int user_to) {
    const char* getMessagesSQL = "SELECT user_from, user_to, hasBeenRead, message FROM messages WHERE ((user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?)) AND hasBeenRead = 0";
    sqlite3_stmt* stmt;
    json messages = json::array();

//    cout << "UNREADDDDDDDDDD:" << endl;

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int from = sqlite3_column_int(stmt, 0);
            int to = sqlite3_column_int(stmt, 1);
            const char* messageText = (const char*)sqlite3_column_text(stmt, 3);
//            cout << from << ' ' << to << ' ' << messageText << endl;
            messages.push_back({{"from", from}, {"to", to}, {"message", messageText}});
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return messages;
}

int getUnreadMessagesCount(int user_from, int user_to) {
    const char* getMessagesSQL = "SELECT COUNT(*) FROM messages WHERE ((user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?)) AND hasBeenRead = 0";
    sqlite3_stmt* stmt;
    int unreadCount = 0;

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            unreadCount = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return unreadCount;
}

json getUserInfo(int user_id) {
    const char* getUserInfo = "SELECT id, firstname, lastname FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    json info;
    if (sqlite3_prepare_v2(db, getUserInfo, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* firstname = (const char*)sqlite3_column_text(stmt, 1);
            const char* lastname = (const char*)sqlite3_column_text(stmt, 2);

            info = {
                    {"firstname", firstname},
                    {"lastname", lastname}
            };
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return info;

}

json getLastMessage(int user_from, int user_to) {
    const char* getLastMessageSQL = "SELECT user_from, user_to, message FROM messages WHERE (user_from = ? AND user_to = ?) OR (user_from = ? AND user_to = ?) ORDER BY id DESC LIMIT 1";
    sqlite3_stmt* stmt;
    json lastMessage;

    if (sqlite3_prepare_v2(db, getLastMessageSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int u_from = sqlite3_column_int(stmt, 0);
            int u_to = sqlite3_column_int(stmt, 1);
            const char *messageText = (const char *) sqlite3_column_text(stmt, 2);
            lastMessage = {
                    {"user_from", u_from},
                    {"user_to", u_to},
                    {"message", messageText}
            };
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return lastMessage;
}
