#include "../include/websocket.h"
#include "../include/globals.h"
#include "../include/message.h"


using namespace std;


// Process private message
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    int user_from = parsed_data["user_from"];
    int user_to = parsed_data["user_to"];
    string message = parsed_data["message"];


    for (const auto& pair : connectedUsers) {
        std::cout << "User ID: " << pair.first << ", Connected: " << std::boolalpha << pair.second << std::endl;
    }

    cout << "User " << user_from << " sent message to " << user_to << endl;

    if (connectedUsers[user_to]){
        json response = {
                {"command", "private_msg"},
                {"user_from", user_from},
                {"user_to", user_to},
                {"message", message}
        };
        ws->publish("user" + to_string(user_to), response.dump());
    } else {
        parsed_data["hasBeenRead"] = 0;
        saveMessage(parsed_data, ws);
    }
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

                connectedUsers[udata->id] = true;

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
        connectedUsers[udata->id] = false;
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

void process_get_unreadMessagesCount(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    auto* udata = ws->getUserData();
    int user_id = parsed_data["user_id"];
    int cur_user = udata->id;
    int messagesCount = getUnreadMessagesCount(user_id, cur_user);

    json response = {
            {"command", "unreadMessagesCount"},
            {"user_id", user_id},
            {"messagesCount", messagesCount}
    };

    ws->send(response.dump(), uWS::OpCode::TEXT);
}

