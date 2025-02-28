#include "../include/message.h"
#include "../include/globals.h"

// Save a message to the database
void saveMessage(json parsed_data, uWS::WebSocket<false, true, UserData> *ws) {
    // Get user data from the WebSocket connection
    auto* udata = ws->getUserData();

    // Extract message details from the parsed JSON data
    int user_from = parsed_data["user_from"];
    int user_to = parsed_data["user_to"];
    int hasBeenRead = parsed_data["hasBeenRead"];
    std::string message = parsed_data["message"];

    // SQL query to insert a message into the 'messages' table
    const char* insertMessageSQL = "INSERT INTO messages (user_from, user_to, hasBeenRead, message) VALUES ($1, $2, $3, $4)";

    // Execute the SQL query with parameters
    PGresult* res = PQexecParams(db, insertMessageSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(hasBeenRead).c_str(), message.c_str()},
                                 nullptr, nullptr, 0);

    // Check if the query execution was successful
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Error inserting message: " << PQerrorMessage(db) << std::endl;
        PQclear(res);
        return;
    }
    PQclear(res);

    // Prepare an acknowledgement message
    json acknowledgement {
            {"command", "sentAcknowledgement"},
            {"user_from", user_from},
            {"user_to", user_to},
            {"message", message}
    };

    // Send the acknowledgement to the sender or publish it to the recipient
    if (user_from != udata->id) {
        ws->publish("user" + std::to_string(user_from), acknowledgement.dump());
    } else {
        ws->send(acknowledgement.dump(), uWS::OpCode::TEXT);
    }
}

// Mark messages as read between two users
void markMessagesAsRead(int user_from, int user_to) {
    // SQL query to update the 'hasBeenRead' flag for messages between two users
    const char* updateMessagesSQL = "UPDATE messages SET hasBeenRead = TRUE WHERE (user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4)";

    PGresult* res = PQexecParams(db, updateMessagesSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Error updating messages: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);
}

// Get all messages between two users
json getAllMessages(int user_from, int user_to) {
    // SQL query to fetch all messages between two users
    const char* getMessagesSQL = "SELECT user_from, user_to, message FROM messages WHERE (user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4)";

    PGresult* res = PQexecParams(db, getMessagesSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    json messages = json::array();

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            int from = std::stoi(PQgetvalue(res, i, 0));
            int to = std::stoi(PQgetvalue(res, i, 1));
            const char* messageText = PQgetvalue(res, i, 2);

            messages.push_back({{"from", from}, {"to", to}, {"message", messageText}});
        }
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }

    PQclear(res);

    return messages;
}

// Get only read messages between two users
json getOnlyReadMessages(int user_from, int user_to) {
    // SQL query to fetch only read messages between two users
    const char* getMessagesSQL = "SELECT user_from, user_to, message FROM messages WHERE ((user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4)) AND hasBeenRead = TRUE";

    PGresult* res = PQexecParams(db, getMessagesSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    json messages = json::array();

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            int user_from = std::stoi(PQgetvalue(res, i, 0));
            int user_to = std::stoi(PQgetvalue(res, i, 1));
            const char* messageText = PQgetvalue(res, i, 2);

            // Add the message to the array
            messages.push_back({{"from", user_from}, {"to", user_to}, {"message", messageText}});
        }
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    return messages;
}

// Get only unread messages between two users
json getOnlyUnreadMessages(int user_from, int user_to) {
    // SQL query to fetch only unread messages between two users
    const char* getMessagesSQL = "SELECT user_from, user_to, message FROM messages WHERE ((user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4)) AND hasBeenRead = FALSE";

    PGresult* res = PQexecParams(db, getMessagesSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    json messages = json::array();

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            int user_from = std::stoi(PQgetvalue(res, i, 0));
            int user_to = std::stoi(PQgetvalue(res, i, 1));
            const char* messageText = PQgetvalue(res, i, 2);

            messages.push_back({{"from", user_from}, {"to", user_to}, {"message", messageText}});
        }
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    return messages;
}

// Get the count of unread messages between two users
int getUnreadMessagesCount(int user_from, int user_to) {
    // SQL query to count unread messages between two users
    const char* getMessagesSQL = "SELECT COUNT(*) FROM messages WHERE ((user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4)) AND hasBeenRead = FALSE";

    PGresult* res = PQexecParams(db, getMessagesSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    int unreadCount = 0; // Variable to store the count of unread messages

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        unreadCount = std::stoi(PQgetvalue(res, 0, 0)); // Extract the count from the query result
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    return unreadCount;
}

// Get user information by ID
json getUserInfo(int user_id) {
    // SQL query to fetch user information by ID
    const char* getUserInfoSQL = "SELECT firstname, lastname FROM users WHERE id = $1";

    PGresult* res = PQexecParams(db, getUserInfoSQL, 1, nullptr,
                                 (const char*[]){std::to_string(user_id).c_str()},
                                 nullptr, nullptr, 0);

    json info; // JSON object to store user information

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        if (PQntuples(res) > 0) {
            const char* firstname = PQgetvalue(res, 0, 0);
            const char* lastname = PQgetvalue(res, 0, 1);
            info = {{"firstname", firstname}, {"lastname", lastname}}; // Store in JSON
        }
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    return info;
}

// Get the last message between two users
json getLastMessage(int user_from, int user_to) {
    // SQL query to fetch the last message between two users
    const char* getLastMessageSQL = "SELECT user_from, user_to, message FROM messages WHERE (user_from = $1 AND user_to = $2) OR (user_from = $3 AND user_to = $4) ORDER BY id DESC LIMIT 1";

    PGresult* res = PQexecParams(db, getLastMessageSQL, 4, nullptr,
                                 (const char*[]){std::to_string(user_from).c_str(), std::to_string(user_to).c_str(), std::to_string(user_to).c_str(), std::to_string(user_from).c_str()},
                                 nullptr, nullptr, 0);

    json lastMessage; // JSON object to store the last message

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        if (PQntuples(res) > 0) {
            int u_from = std::stoi(PQgetvalue(res, 0, 0));
            int u_to = std::stoi(PQgetvalue(res, 0, 1));
            const char* messageText = PQgetvalue(res, 0, 2);

            // Store the message in JSON
            lastMessage = {{"user_from", u_from}, {"user_to", u_to}, {"message", messageText}};
        }
    } else {
        std::cerr << "Error executing query: " << PQerrorMessage(db) << std::endl;
    }
    PQclear(res);

    return lastMessage;
}