#include "../include/websocket.h"
#include "../include/globals.h"

using namespace std;

// Process private message
void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Extract sender, recipient, and message from the parsed JSON data
    int user_from = parsed_data["user_from"];
    int user_to = parsed_data["user_to"];
    string message = parsed_data["message"];

    // Log the message sending event
    cout << "User " << user_from << " sent message to " << user_to << endl;

    // If the recipient is connected, send the message directly
    if (connectedUsers[user_to]) {
        json response = {
                {"command", "private_msg"},
                {"user_from", user_from},
                {"user_to", user_to},
                {"message", message}
        };
        ws->publish("user" + to_string(user_to), response.dump());
    } else {
        // If the recipient is offline, save the message to the database
        parsed_data["hasBeenRead"] = 0;
        saveMessage(parsed_data, ws);
    }
}

// Process registration
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();
    udata->firstname = parsed_data["firstname"];
    udata->lastname = parsed_data["lastname"];
    udata->nickname = parsed_data["nickname"];
    udata->password = parsed_data["password"];
    udata->id = latest_user_id++; // Assign a new user ID
    udata->hisChat = "user" + to_string(udata->id); // Create a private channel for the user
    cout << "New user registered: " << udata->id << endl;

    // Insert the new user into the PostgreSQL database
    const char* insertUserSQL = "INSERT INTO users (id, firstname, lastname, nickname, password) VALUES ($1, $2, $3, $4, $5)";
    PGresult* res = PQexecParams(db, insertUserSQL, 5, nullptr,
                                 (const char*[]){to_string(udata->id).c_str(), udata->firstname.c_str(), udata->lastname.c_str(), udata->nickname.c_str(), udata->password.c_str()},
                                 nullptr, nullptr, 0);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Error inserting user: " << PQerrorMessage(db) << endl;
    }
    PQclear(res);

    // Send a response to the client
    json response = {
            {"command", "registered"},
            {"user_id", udata->id}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);

    // Subscribe the user to the public channel and their private channel
    ws->subscribe("public");
    ws->subscribe(udata->hisChat);

    // Notify all users about the new registration
    ws->publish("public", "New user " + to_string(udata->id) + " registered with name " + udata->nickname);

    // Add the user to the connected users map
    connectedUsers[udata->id] = udata;

    // Notify all users to refresh the user list
    json notifyAll = {
            {"command", "user_list_refresh"}
    };
    ws->publish("public", notifyAll.dump());
}

// Process login and send previous messages
void process_login(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Extract nickname and password from the parsed JSON data
    string nickname = parsed_data["nickname"];
    string password = parsed_data["password"];

    // Query the database for the user
    const char* sql = "SELECT id, nickname, password FROM users WHERE nickname = $1";
    PGresult* res = PQexecParams(db, sql, 1, nullptr, (const char*[]){nickname.c_str()}, nullptr, nullptr, 0);

    // Check if the query execution was successful
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        // Extract user data from the query result
        int id = stoi(PQgetvalue(res, 0, 0));
        const char* dbName = PQgetvalue(res, 0, 1);
        const char* dbPassword = PQgetvalue(res, 0, 2);

        // Validate the password
        if (password == dbPassword && nickname == dbName) {
            auto* udata = ws->getUserData();
            udata->id = id;
            udata->nickname = nickname;
            udata->password = password;
            udata->hisChat = "user" + to_string(udata->id);

            // Send a response to the client
            json response = {
                    {"command", "logged_in"},
                    {"user_id", udata->id},
                    {"name", udata->nickname},
            };
            ws->send(response.dump(), uWS::OpCode::TEXT);

            // Subscribe the user to the public channel and their private channel
            ws->subscribe("public");
            ws->subscribe(udata->hisChat);

            // Add the user to the connected users map
            connectedUsers[udata->id] = true;

            // Log the login event
            cout << "User " << udata->nickname << " logged in with ID: " << udata->id << endl;
            ws->publish("public", "User " + udata->nickname + " logged in");
        } else if (password != dbPassword) {
            // Send a login failed response (wrong password)
            json response = {{"command", "login_failed"}, {"fail", "password"}};
            ws->send(response.dump(), uWS::OpCode::TEXT);
        }
    } else {
        // Send a login failed response (user not found)
        json response = {{"command", "login_failed"}, {"fail", "name"}};
        ws->send(response.dump(), uWS::OpCode::TEXT);
    }
    PQclear(res);
}

// Process logout
void process_logout(uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();
    if (udata->id != 0) {
        // Log the logout event
        cout << "User " << udata->id << " logged out" << endl;

        // Mark the user as disconnected
        connectedUsers[udata->id] = false;

        // Clear user data
        udata->id = 0;
        udata->nickname = "";
        udata->password = "";
        udata->hisChat = "";
    }

    // Notify all users about the logout
    ws->publish("public", "User " + udata->nickname + " logged out");
}

// Process user list request from the database
void process_user_list_from_db(uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();

    // Query the database for all users
    const char* sql = "SELECT id, firstname, lastname, nickname FROM users";
    PGresult* res = PQexec(db, sql);

    json userList = json::array();

    // Check if the query execution was successful
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            // Extract user data from the query result
            int id = stoi(PQgetvalue(res, i, 0));
            const char* firstname = PQgetvalue(res, i, 1);
            const char* lastname = PQgetvalue(res, i, 2);
            const char* nickname = PQgetvalue(res, i, 3);

            // Exclude the current user from the list
            if (id != udata->id) {
                userList.push_back({{"id", id}, {"nickname", nickname}, {"firstname", firstname}, {"lastname", lastname}});
            }
        }
    } else {
        cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    // Send the user list to the client
    json response = {
            {"command", "user_list"},
            {"users", userList}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
}

// Process get messages
void process_get_messages(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();
    int user_from = udata->id;
    int user_to = parsed_data["user_id"];
    int flagMarkMessagesAsRead = parsed_data["markMessagesAsRead"];

    // If the flag is set, fetch read and unread messages separately
    if (flagMarkMessagesAsRead) {
        json readMessages = getOnlyReadMessages(user_from, user_to);
        json unreadMessages = getOnlyUnreadMessages(user_from, user_to);
        markMessagesAsRead(user_from, user_to);

        // Send read messages to the client
        json readMessagesResponse = {
                {"command", "onlyReadMessages"},
                {"user_id", user_to},
                {"messages", readMessages}
        };
        ws->send(readMessagesResponse.dump(), uWS::OpCode::TEXT);

        // Send unread messages to the client
        json unreadMessagesResponse = {
                {"command", "onlyUnreadMessages"},
                {"user_id", user_to},
                {"messages", unreadMessages}
        };
        ws->send(unreadMessagesResponse.dump(), uWS::OpCode::TEXT);
        return;
    }

    // Fetch all messages between the users
    json messages = getAllMessages(udata->id, user_to);

    // Send the messages to the client
    json response = {
            {"command", "onlyReadMessages"},
            {"user_id", user_to},
            {"messages", messages}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
}

// Process get last message
void process_get_last_message(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();
    int user_from = udata->id;
    int user_to = parsed_data["user_id"];

    // Fetch the last message between the users
    json lastMessage = getLastMessage(user_from, user_to);
    if (!lastMessage.empty()) {
        // Fetch sender's information
        json userInfo_from = getUserInfo(lastMessage["user_from"]);

        // Send the last message to the client
        json response = {
                {"command", "last_message"},
                {"user_from", lastMessage["user_from"]},
                {"user_to", lastMessage["user_to"]},
                {"userInfo_from", userInfo_from},
                {"message", lastMessage["message"]}
        };
        ws->send(response.dump(), uWS::OpCode::TEXT);
    } else {
        // Send an empty response if no messages exist
        json response = {
                {"command", "last_message"},
                {"user_from", udata->id},
                {"user_to", user_to},
                {"userInfo_from", ""},
                {"message", ""}
        };
        ws->send(response.dump(), uWS::OpCode::TEXT);
    }
}

// Process get unread messages count
void process_get_unreadMessagesCount(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();
    int user_from = parsed_data["user_id"];
    int user_to = udata->id;

    // Fetch the count of unread messages
    int messagesCount = getUnreadMessagesCount(user_from, user_to);

    // Send the count to the client
    json response = {
            {"command", "unreadMessagesCount"},
            {"user_id", user_from},
            {"messagesCount", messagesCount}
    };
    ws->send(response.dump(), uWS::OpCode::TEXT);
}