#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <sqlite3.h>
using namespace std;
using json = nlohmann::json;

// User data structure
struct UserData {
    int id;
    string firstname;
    string lastname;
    string nickname;
    string password;
    string hisChat;
};

// Global container to store users
unordered_map<int, UserData*> connectedUsers;

// Initialize SQLite database
sqlite3* db;

int latest_user_id;

void initDatabase() {
    if (sqlite3_open("chat.db", &db)) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db) << endl;
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
        cerr << "Error creating users table: " << errMsg << endl;
        sqlite3_free(errMsg);
        exit(1);
    }

    if (sqlite3_exec(db, createMessagesTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "Error creating messages table: " << errMsg << endl;
        sqlite3_free(errMsg);
        exit(1);
    }
}

void saveMessage(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    cout << parsed_data["user_from"] << " " << parsed_data["user_to"] << " " << endl;
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

        ws->publish("user" + to_string(user_from), acknowledgement.dump());

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cerr << "Error inserting message: " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing insert statement: " << sqlite3_errmsg(db) << endl;
    }
}

//void saveUnreadMessage(json parsed_data) {
//
//    cout << parsed_data["user_from"] << " " << parsed_data["user_to"] << " " << endl;
//    int user_from = parsed_data["user_from"];
//    int user_to = parsed_data["user_to"];
//    string message = parsed_data["message"];
//
//    const char* insertMessageSQL = "INSERT INTO unreadMessages (user_from, user_to, message) VALUES (?, ?, ?)";
//    sqlite3_stmt* stmt;
//    if (sqlite3_prepare_v2(db, insertMessageSQL, -1, &stmt, nullptr) == SQLITE_OK) {
//        sqlite3_bind_int(stmt, 1, user_from);
//        sqlite3_bind_int(stmt, 2, user_to);
//        sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_STATIC);
//
//        if (sqlite3_step(stmt) != SQLITE_DONE) {
//            cerr << "Error inserting message: " << sqlite3_errmsg(db) << endl;
//        }
//        sqlite3_finalize(stmt);
//    } else {
//        cerr << "Error preparing insert statement: " << sqlite3_errmsg(db) << endl;
//    }
//
//}

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
            cout << from << ' ' << to << ' ' << messageText << endl;
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

    cout << "RRRRRRREADDDDDDDDDD:" << endl;

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int from = sqlite3_column_int(stmt, 0);
            int to = sqlite3_column_int(stmt, 1);
            const char* messageText = (const char*)sqlite3_column_text(stmt, 3);
            cout << from << ' ' << to << ' ' << messageText << endl;
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

    cout << "UNREADDDDDDDDDD:" << endl;

    if (sqlite3_prepare_v2(db, getMessagesSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_int(stmt, 3, user_to);
        sqlite3_bind_int(stmt, 4, user_from);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int from = sqlite3_column_int(stmt, 0);
            int to = sqlite3_column_int(stmt, 1);
            const char* messageText = (const char*)sqlite3_column_text(stmt, 3);
            cout << from << ' ' << to << ' ' << messageText << endl;
            messages.push_back({{"from", from}, {"to", to}, {"message", messageText}});
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return messages;
}

// Get the last user ID from the database
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
        cerr << "Error executing query: " << sqlite3_errmsg(db) << endl;
    }

    return lastId;
}

// Process private message
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    int user_from = parsed_data["user_from"];
    int user_to = parsed_data["user_to"];
    string message = parsed_data["message"];


    cout << "User " << user_from << " sent message to " << user_to << endl;
    json response = {
            {"command", "private_msg"},
            {"user_from", user_from},
            {"user_to", user_to},
            {"message", message}
    };
    ws->publish("user" + to_string(user_to), response.dump());

}



// Process registration
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    udata->firstname = parsed_data["firstname"];
    udata->lastname = parsed_data["lastname"];
    udata->nickname = parsed_data["nickname"];
    udata->password = parsed_data["password"];
    udata->id = latest_user_id++;
    udata->hisChat = "user" + to_string(udata->id);
    cout << "New user registered: " << udata->id << endl;

    const char* insertUserSQL = "INSERT INTO users (id, firstname, lastname, nickname, password) VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertUserSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, udata->id);
        sqlite3_bind_text(stmt, 2, udata->firstname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, udata->lastname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, udata->nickname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, udata->password.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cerr << "Error inserting user: " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing insert statement: " << sqlite3_errmsg(db) << endl;
    }

    json response = {
            {"command", "registered"},
            {"user_id", udata->id}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);

    ws->subscribe("public");
    ws->subscribe(udata->hisChat);

    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->nickname);

    connectedUsers[udata->id] = udata;

    // Notify all users to refresh the user list
    json notifyAll = {
            {"command", "user_list_refresh"}
    };
    ws->publish("public", notifyAll.dump());
}

// Process login and send previous messages
void process_login(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    string nickname = parsed_data["nickname"];
    string password = parsed_data["password"];
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, nickname, password FROM users WHERE nickname = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nickname.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* dbName = (const char*)sqlite3_column_text(stmt, 1);
            const char* dbPassword = (const char*)sqlite3_column_text(stmt, 2);

            if (password == dbPassword && nickname == dbName) {
                auto* udata = ws->getUserData();
                udata->id = id;
                udata->nickname = nickname;
                udata->password = password;
                udata->hisChat = "user" + to_string(udata->id);

                json response = {
                        {"command", "logged_in"},
                        {"user_id", udata->id},
                        {"name", udata->nickname},
                };
                ws->send(response.dump(), uWS::OpCode::TEXT);
                ws->subscribe("public");
                ws->subscribe(udata->hisChat);

                cout << "User " << udata->nickname << " logged in with ID: " << udata->id << endl;
                ws->publish("public", "User " + udata->nickname + " logged in");

                connectedUsers[udata->id] = udata;
            } else if (password != dbPassword) {
                json response = {{"command", "login_failed"}, {"fail", "password"}};
                ws->send(response.dump(), uWS::OpCode::TEXT);
            }
        } else {
            json response = {{"command", "login_failed"}, {"fail", "name"}};
            ws->send(response.dump(), uWS::OpCode::TEXT);
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing select statement: " << sqlite3_errmsg(db) << endl;
    }
}

// Process logout
void process_logout(uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    if (udata->id != 0) {
        cout << "User " << udata->id << " logged out" << endl;
        connectedUsers.erase(udata->id); // Remove user from global container
        udata->id = 0;
        udata->nickname = "";
        udata->password = "";
        udata->hisChat = "";
    }
    ws->publish("public", "User " + udata->nickname + " logged out");
}

// Process user list request from the database
void process_user_list_from_db(uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    const char* sql = "SELECT id, firstname, lastname, nickname FROM users";
    sqlite3_stmt* stmt;
    json userList = json::array();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* firstname = (const char*)sqlite3_column_text(stmt, 1);
            const char* lastname = (const char*)sqlite3_column_text(stmt, 2);
            const char* nickname = (const char*)sqlite3_column_text(stmt, 3);
            if (id != udata->id) {
                userList.push_back({{"id", id}, {"nickname", nickname}, {"firstname", firstname}, {"lastname", lastname}});
            }
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
    }

    json response = {
            {"command", "user_list"},
            {"users", userList}
    };

    ws->send(response.dump(), uWS::OpCode::TEXT);
}

// Process get messages
void process_get_messages(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_to = parsed_data["user_id"];
    int flagMarkMessagesAsRead = parsed_data["markMessagesAsRead"];

    if (flagMarkMessagesAsRead) {
        json readMessages = getOnlyReadMessages(udata->id, user_to);
        json unreadMessages = getOnlyUnreadMessages(udata->id, user_to);
        markMessagesAsRead(udata->id, user_to);
        json readMessagesResponse = {
                {"command", "onlyReadMessages"},
                {"user_id", user_to},
                {"messages", readMessages}
        };
        ws->send(readMessagesResponse.dump(), uWS::OpCode::TEXT);

        json unreadMessagesResponse = {
                {"command", "onlyUnreadMessages"},
                {"user_id", user_to},
                {"messages", unreadMessages}
        };
        ws->send(unreadMessagesResponse.dump(), uWS::OpCode::TEXT);
        return;
    }

    json messages = getAllMessages(udata->id, user_to);

    json response = {
            {"command", "onlyReadMessages"},
            {"user_id", user_to},
            {"messages", messages}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
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

//void process_getUserInfo (json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
//    int user_id = parsed_data[user_id];
//    json info = getUserInfo(user_id);
//    if (!info.empty()) {
//        json response = {
//                {"command", "userInfo"},
//                {"info", info}
//        };
//        ws->send(response.dump(), uWS::OpCode::TEXT)
//    }
//}


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

void process_get_last_message(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_to = parsed_data["user_id"];
    json lastMessage = getLastMessage(udata->id, user_to);
    if (!lastMessage.empty()) {
        json userInfo_from = getUserInfo(lastMessage["user_from"]);
        json response = {
                {"command", "last_message"},
                {"user_from", lastMessage["user_from"]},
                {"user_to", lastMessage["user_to"]},
                {"userInfo_from", userInfo_from},
                {"message", lastMessage["message"]}
        };
        ws->send(response.dump(), uWS::OpCode::TEXT);
    } else {
        json response = {
                {"command", "last_message"},
                {"user_from", user_to},
                {"user_to", udata->id},
                {"userInfo_from", ""},
                {"message", ""}
        };
        ws->send(response.dump(), uWS::OpCode::TEXT);
    }
}

// Function to print table contents
void printUsersTable() {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT * FROM users";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        int cols = sqlite3_column_count(stmt);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int col = 0; col < cols; col++) {
                const char* colName = sqlite3_column_name(stmt, col);
                const char* colText = (const char*)sqlite3_column_text(stmt, col);
                cout << colName << ": " << (colText ? colText : "NULL") << " | ";
            }
            cout << endl;
        }

        sqlite3_finalize(stmt);
    } else {
        cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
    }
}

int main() {
    initDatabase();
    latest_user_id = getLastUserId(db) + 1;

    uWS::App().ws<UserData>("/*", {
            .idleTimeout = 16,
            .upgrade = nullptr,
            .open = [](auto* ws) {
                ws->send("Welcome to BreezeTalk", uWS::OpCode::TEXT);
                cout << "Users table contents:" << endl;
                printUsersTable();
            },
            .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                json parsed_data = json::parse(message);
                if (parsed_data["command"] == "private_msg") {
                    process_private_msg(parsed_data, ws);
                } else if (parsed_data["command"] == "register") {
                    process_registration(parsed_data, ws);
                } else if (parsed_data["command"] == "login") {
                    process_login(parsed_data, ws);
                } else if (parsed_data["command"] == "logout"){
                    process_logout(ws);
                } else if (parsed_data["command"] == "user_list_db") {
                    process_user_list_from_db(ws);
                } else if (parsed_data["command"] == "get_messages") {
                    process_get_messages(parsed_data, ws);
                } else if (parsed_data["command"] == "get_last_message") {
                    process_get_last_message(parsed_data, ws);
                } else if (parsed_data["command"] == "save_read_private_msg") {
                    saveMessage(parsed_data, ws);
                }
            },
            .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
                auto* data = ws->getUserData();
                cout << "User disconnected: " << data->id << endl;
                connectedUsers.erase(data->id);
            }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            cout << "Listening on port " << 9001 << endl;
        }
    }).run();

    sqlite3_close(db);

    return 0;
}
