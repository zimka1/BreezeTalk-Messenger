#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include "user.h"
#include "message.h"
#include "database.h"
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

void process_private_msg(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void process_registration(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void process_login(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void process_logout(uWS::WebSocket<false, true, UserData> *ws);
void process_user_list_from_db(uWS::WebSocket<false, true, UserData> *ws);
void process_get_messages(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void process_get_last_message(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);
void process_get_unreadMessagesCount(json parsed_data, uWS::WebSocket<false, true, UserData> *ws);

#endif