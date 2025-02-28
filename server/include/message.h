#ifndef MESSAGE_H
#define MESSAGE_H

#include <nlohmann/json.hpp>
#include <uwebsockets/App.h>
#include "user.h"

using json = nlohmann::json;

// Function declarations
void saveMessage(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void markMessagesAsRead(int user_from, int user_to);
json getAllMessages(int user_from, int user_to);
json getOnlyReadMessages(int user_from, int user_to);
json getOnlyUnreadMessages(int user_from, int user_to);
int getUnreadMessagesCount(int user_from, int user_to);
json getUserInfo(int user_id);
json getLastMessage(int user_from, int user_to);

#endif