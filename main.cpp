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
void initDatabase() {
    if (sqlite3_open("chat.db", &db)) {
        cerr << "Failed to open database: " << sqlite3_errmsg(db) << endl;
        exit(1);
    }

    const char* createUsersTable = "CREATE TABLE IF NOT EXISTS users ("
                                   "id INTEGER PRIMARY KEY,"
                                   "name TEXT NOT NULL,"
                                   "password TEXT NOT NULL);";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, createUsersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "Error creating users table: " << errMsg << endl;
        sqlite3_free(errMsg);
        exit(1);
    }
}

// Process public message
void process_public_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    string message = parsed_data["text"];
    ws->publish("public", udata->name + ": " + message);
}

// Process private message
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_to = parsed_data["user_to"];
    string message = parsed_data["text"];

    cout << "User " << udata->id << " sent a message to " << user_to << endl;
    ws->publish("user" + to_string(user_to), udata->name + ": " + message);
}

// Process registration
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    udata->name = parsed_data["name"];
    udata->password = parsed_data["password"];

    // Save user data to the database
    const char* insertUserSQL = "INSERT INTO users (id, name, password) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, insertUserSQL, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, udata->id);
    sqlite3_bind_text(stmt, 2, udata->name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, udata->password.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Send user ID back to client
    json response = {
            {"command", "registered"},
            {"user_id", udata->id}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);

    // Notify about new user registration
    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->name);

    // Add user to global container
    connectedUsers[udata->id] = udata;
}

// Process user list request
void process_user_list(uWS::WebSocket<false, true, UserData> *ws) {
    json userList = json::array();
    for (const auto& userPair : connectedUsers) {
        userList.push_back({{"id", userPair.second->id}, {"name", userPair.second->name}, {"password", userPair.second->password}});
    }
    ws->send(userList.dump(), uWS::OpCode::TEXT);
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
                std::cout << colName << ": " << (colText ? colText : "NULL") << " | ";
            }
            std::cout << std::endl;
        }

        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Error executing query: " << sqlite3_errmsg(db) << std::endl;
    }
}

// Function to clear the users table
void clearUsersTable() {
    const char* clearTableSQL = "DELETE FROM users";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, clearTableSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "Error clearing users table: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

int main() {
    // Initialize database
    initDatabase();

    int latest_user_id = 10; // Incremented each time a new user registers

    uWS::App().ws<UserData>("/*", {
            .idleTimeout = 16,
            /* Handlers */
            .upgrade = nullptr,
            // User connected to the server
            .open = [&latest_user_id](auto* ws) {
                // Assign user id and default name
                auto* data = ws->getUserData();
                data->id = latest_user_id++;
                data->name = "unnamed";
                data->hisChat = "user" + to_string(data->id);
                cout << "New user connected: " << data->id << endl;
                ws->send("Welcome to BreezeTalk", uWS::OpCode::TEXT);

                ws->subscribe("public");
                ws->subscribe(data->hisChat);

                // Print table contents
                std::cout << "Users table contents:" << std::endl;
                printUsersTable();
            },
            // User sent data to the server
            .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                json parsed_data = json::parse(message);
                if (parsed_data["command"] == "public_msg") {
                    process_public_msg(parsed_data, ws);
                } else if (parsed_data["command"] == "private_msg") {
                    process_private_msg(parsed_data, ws);
                } else if (parsed_data["command"] == "register") {
                    process_registration(parsed_data, ws);
                } else if (parsed_data["command"] == "user_list") {
                    process_user_list(ws);
                }
            },
            // User disconnected from the server
            .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
                auto* data = ws->getUserData();
                cout << "User disconnected: " << data->id << endl;
                connectedUsers.erase(data->id); // Remove user from global container
            }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();

    // Clear the users table when the server shuts down
    clearUsersTable();

    // Close database when server shuts down
    sqlite3_close(db);

    return 0;
}
