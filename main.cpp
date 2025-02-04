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
    string name;
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
                                   "name TEXT NOT NULL,"
                                   "password TEXT NOT NULL);";

    const char* createMessagesTable = "CREATE TABLE IF NOT EXISTS messages ("
                                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "user_from INTEGER,"
                                      "user_to INTEGER,"
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

void saveMessage(int user_from, int user_to, const string& message) {
    const char* insertMessageSQL = "INSERT INTO messages (user_from, user_to, message) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertMessageSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_from);
        sqlite3_bind_int(stmt, 2, user_to);
        sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            cerr << "Error inserting message: " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);
    } else {
        cerr << "Error preparing insert statement: " << sqlite3_errmsg(db) << endl;
    }
}

json getMessages(int user_from, int user_to) {
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

// Process public message
void process_public_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    string message = parsed_data["text"];
    for (const auto& user : connectedUsers) {
        if (udata->id != user.first) {
            saveMessage(udata->id, user.first, udata->name + ": " +  message);
        }
    }

    ws->publish("public", udata->name + ": " + message);
}

// Process private message
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_to = parsed_data["user_to"];
    string message = parsed_data["text"];
    saveMessage(udata->id, user_to, message);

    cout << "User " << udata->id << " sent a message to " << user_to << endl;
    json response = {
            {"command", "private_msg"},
            {"user_from", udata->id},
            {"user_to", user_to},
            {"message", message}
    };
    ws->publish("user" + to_string(user_to), response.dump());
}

// Process registration
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    udata->name = parsed_data["name"];
    udata->password = parsed_data["password"];
    udata->id = latest_user_id++;
    udata->hisChat = "user" + to_string(udata->id);
    cout << "New user registered: " << udata->id << endl;

    const char* insertUserSQL = "INSERT INTO users (id, name, password) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertUserSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, udata->id);
        sqlite3_bind_text(stmt, 2, udata->name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, udata->password.c_str(), -1, SQLITE_STATIC);
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

    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->name);

    connectedUsers[udata->id] = udata;

    // Notify all users to refresh the user list
    json notifyAll = {
            {"command", "user_list_refresh"}
    };
    ws->publish("public", notifyAll.dump());
}

// Process login and send previous messages
void process_login(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    string name = parsed_data["name"];
    string password = parsed_data["password"];
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name, password FROM users WHERE name = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* dbName = (const char*)sqlite3_column_text(stmt, 1);
            const char* dbPassword = (const char*)sqlite3_column_text(stmt, 2);

            if (password == dbPassword && name == dbName) {
                auto* udata = ws->getUserData();
                udata->id = id;
                udata->name = name;
                udata->password = password;
                udata->hisChat = "user" + to_string(udata->id);

                json response = {
                        {"command", "logged_in"},
                        {"user_id", udata->id},
                        {"name", udata->name},
                        {"messages", getMessages(udata->id, udata->id)}
                };
                ws->send(response.dump(), uWS::OpCode::TEXT);
                ws->subscribe("public");
                ws->subscribe(udata->hisChat);

                cout << "User " << udata->name << " logged in with ID: " << udata->id << endl;
                ws->publish("public", "User " + udata->name + " logged in");

                connectedUsers[udata->id] = udata;
            } else {
                json response = {{"command", "login_failed"}, {"message", "Invalid password"}};
                ws->send(response.dump(), uWS::OpCode::TEXT);
            }
        } else {
            json response = {{"command", "login_failed"}, {"message", "User not found"}};
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
        udata->name = "";
        udata->password = "";
        udata->hisChat = "";
    }
    ws->publish("public", "User " + udata->name + " logged out");
}

// Process user list request from the database
void process_user_list_from_db(uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    const char* sql = "SELECT id, name FROM users";
    sqlite3_stmt* stmt;
    json userList = json::array();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const char* name = (const char*)sqlite3_column_text(stmt, 1);
            if (id != udata->id) {
                userList.push_back({{"id", id}, {"name", name}});
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
    int user_id = parsed_data["user_id"];
    json messages = getMessages(udata->id, user_id);

    json response = {
            {"command", "messages"},
            {"user_id", user_id},
            {"messages", messages}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
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
                if (parsed_data["command"] == "public_msg") {
                    process_public_msg(parsed_data, ws);
                } else if (parsed_data["command"] == "private_msg") {
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
