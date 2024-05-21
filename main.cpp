#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
using namespace std;
using json = nlohmann::json;

// Define user data structure
struct UserData {
    int id;
    string name;
    string hisChat;
};

// Global container to store users
unordered_map<int, UserData*> connectedUsers;

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

    cout << "User " << udata->id << " send message to " << user_to << endl;
    ws->publish("user" + to_string(user_to), udata->name + ": " + message);
}

// Process registration
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    udata->name = parsed_data["name"];

    // Send user ID back to client
    json response = {
            {"command", "registered"},
            {"user_id", udata->id}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);

    // Broadcast new user registration
    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->name);

    // Add user to global container
    connectedUsers[udata->id] = udata;
}

// Process user list request
void process_user_list(uWS::WebSocket<false, true, UserData> *ws) {
    json userList = json::array();
    for (const auto& userPair : connectedUsers) {
        userList.push_back({{"id", userPair.second->id}, {"name", userPair.second->name}});
    }
    ws->send(userList.dump(), uWS::OpCode::TEXT);
}

int main() {
    int latest_user_id = 10; // Incremented each time

    uWS::App().ws<UserData>("/*", {
            .idleTimeout = 16,
            /* Handlers */
            .upgrade = nullptr,
            // someone connected to the server
            .open = [&latest_user_id](auto* ws) {
                // Assign user: id, default name
                auto* data = ws->getUserData();
                data->id = latest_user_id++;
                data->name = "unnamed";
                data->hisChat = "user" + to_string(data->id);
                cout << "New user connected: " << data->id << endl;
                ws->send("Welcome to BreezeTalk", uWS::OpCode::TEXT);

                ws->subscribe("public");
                ws->subscribe(data->hisChat);
            },
            // someone sent data packet to the server
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
            // someone disconnected from the server
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
}
