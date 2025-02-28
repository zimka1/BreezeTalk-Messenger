#include "../include/websocket.h"
#include "../include/globals.h"
#include <unordered_map>

// Map command strings to integers for use in the switch statement
const std::unordered_map<std::string, int> COMMAND_MAP = {
        {"private_msg", 1},       // Command for sending a private message
        {"register", 2},          // Command for user registration
        {"login", 3},             // Command for user login
        {"logout", 4},            // Command for user logout
        {"user_list_db", 5},      // Command for fetching the list of users from the database
        {"get_messages", 6},      // Command for retrieving messages between users
        {"get_last_message", 7},  // Command for fetching the last message between users
        {"save_read_private_msg", 8}, // Command for saving a private message and marking it as read
        {"get_unread_messages_count", 9} // Command for getting the count of unread messages
};

int main() {
    // Initialize the database (create tables if they don't exist and establish a connection)
    initDatabase();

    // Get the latest user ID from the database and increment it by 1 for the next user
    latest_user_id = getLastUserId(db) + 1;

    // Start the WebSocket server
    uWS::App().ws<UserData>("/*", {
            .idleTimeout = 16, // Timeout for idle connections (in seconds)
            .upgrade = nullptr, // No custom upgrade logic
            .open = [](auto* ws) {
                // Send a welcome message to the client when the connection is established
                ws->send("Welcome to BreezeTalk", uWS::OpCode::TEXT);

                // Print the contents of the users table for debugging purposes
                std::cout << "Users table contents:" << std::endl;
                printUsersTable(db); // Pass the PostgreSQL connection object
            },
            .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                // Parse the incoming message as JSON
                json parsed_data = json::parse(message);

                // Get the command from the parsed data
                std::string command = parsed_data["command"];

                // Use the map to convert the command string to an integer
                auto it = COMMAND_MAP.find(command);
                if (it == COMMAND_MAP.end()) {
                    std::cerr << "Unknown command: " << command << std::endl;
                    return;
                }

                switch (it->second) {
                    case 1: // private_msg
                        process_private_msg(parsed_data, ws);
                        break;
                    case 2: // register
                        process_registration(parsed_data, ws);
                        break;
                    case 3: // login
                        process_login(parsed_data, ws);
                        break;
                    case 4: // logout
                        process_logout(ws);
                        break;
                    case 5: // user_list_db
                        process_user_list_from_db(ws);
                        break;
                    case 6: // get_messages
                        process_get_messages(parsed_data, ws);
                        break;
                    case 7: // get_last_message
                        process_get_last_message(parsed_data, ws);
                        break;
                    case 8: // save_read_private_msg
                        saveMessage(parsed_data, ws);
                        break;
                    case 9: // get_unread_messages_count
                        process_get_unreadMessagesCount(parsed_data, ws);
                        break;
                    default:
                        // Handle any unhandled commands
                        std::cerr << "Unhandled command: " << command << std::endl;
                        break;
                }
            },
            .close = [](auto *ws, int /*code*/, std::string_view /*message*/) {
                // Handle user disconnection
                auto* data = ws->getUserData();
                std::cout << "User disconnected: " << data->id << std::endl;

                // Remove the user from the connected users map
                connectedUsers.erase(data->id);
            }
    }).listen(9001, [](auto *listen_socket) {
        // Callback when the server starts listening on port 9001
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run(); // Start the WebSocket server event loop

    PQfinish(db);

    return 0;
}