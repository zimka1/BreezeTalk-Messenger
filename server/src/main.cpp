#include "../include/websocket.h"
#include "../include/database.h"
#include "../include/globals.h"


int main() {
    initDatabase();
    latest_user_id = getLastUserId(db) + 1;

    uWS::App().ws<UserData>("/*", {
            .idleTimeout = 16,
            .upgrade = nullptr,
            .open = [](auto* ws) {
                ws->send("Welcome to BreezeTalk", uWS::OpCode::TEXT);
                std::cout << "Users table contents:" << std::endl;
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
                } else if (parsed_data["command"] == "logout") {
                    process_logout(ws);
                } else if (parsed_data["command"] == "user_list_db") {
                    process_user_list_from_db(ws);
                } else if (parsed_data["command"] == "get_messages") {
                    process_get_messages(parsed_data, ws);
                } else if (parsed_data["command"] == "get_last_message") {
                    process_get_last_message(parsed_data, ws);
                } else if (parsed_data["command"] == "save_read_private_msg") {
                    saveMessage(parsed_data, ws);
                } else if (parsed_data["command"] == "get_unread_messages_count") {
                    process_get_unreadMessagesCount(parsed_data, ws);
                }
            },
            .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
                auto* data = ws->getUserData();
                std::cout << "User disconnected: " << data->id << std::endl;
                connectedUsers.erase(data->id);
            }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();

    sqlite3_close(db);

    return 0;
}