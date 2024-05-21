#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

// 3. Users will have names
// 4. How to know who is connected to the server?

struct UserData {
    int id;
    string name;
    string hisChat;
};


// [USER 17] =server> {"command": "public_msg", "text": "a"}
// [SERVER] =everyone> {"command": "public_msg", "text":"a", "user_from": 17}
void process_public_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws){
    auto* udata = ws->getUserData();
    string message = parsed_data["text"].dump();
//    json payload = {
//            {"command", "public_msg"},
//            {"text", parsed_data["text"]},
//            {"user_from", udata->id}
//    };
//    ws->publish("public", payload.dump());
    ws->publish("public", udata->name + ": " + message);
}

// [USER 17] =server> {"command": "private_msg", "text": "hi", "user_to": 23}
// [SERVER] =user23> {"command": "private_msg", "text": "hi", "user_from": 17}
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_to = parsed_data["user_to"];
    string message = parsed_data["text"].dump();
//    json payload = {
//            {"command", "private_msg"},
//            {"text", parsed_data["text"]},
//            {"user_from", udata->id}
//    };
    cout << "User " << udata->id << " send message to " << user_to << endl;
//    ws->publish("user" + to_string(user_to), payload.dump());
    ws->publish("user" + to_string(user_to), udata->name + ": " + message);

}

void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws){
    auto* udata = ws->getUserData();
    udata->name = parsed_data["name"];

    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->name);
}

int main() {
    int latest_user_id = 10; // incremented each time

    vector <UserData> Users;

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

                json welcome_message = {
                        {"command", "welcome"},
                        {"user_id", data->id}
                };
                ws->send(welcome_message.dump(), uWS::OpCode::TEXT);

                ws->subscribe("public");
                ws->subscribe(data->hisChat);
            },
            // someone sent data packet to the server
            .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                json parsed_data = json::parse(message);
                if (parsed_data["command"] == "public_msg") {
                    process_public_msg(parsed_data, ws);
                }
                if (parsed_data["command"] == "private_msg"){
                    process_private_msg(parsed_data, ws);
                }
                if (parsed_data["command"] == "register"){
                    process_registration(parsed_data, ws);
                }
            },
            // someone disconnected from the server
            .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
                /* You may access ws->getUserData() here */
            }
    }).listen(9001, [](auto *listen_socket) {
        if (listen_socket) {
            std::cout << "Listening on port " << 9001 << std::endl;
        }
    }).run();
}
