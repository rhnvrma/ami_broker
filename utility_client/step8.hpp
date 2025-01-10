#ifndef WEBSOCKET_CLIENT_EXAMPLE_H
#define WEBSOCKET_CLIENT_EXAMPLE_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "json.hpp"
#include "MarketDataFeedV3.pb.h"
#include <iostream>
#include <unordered_map>

using json = nlohmann::json;

struct Ohlcv {
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int64_t volume = 0;
    void print() const;
};

extern std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg);
void on_open(client* c, websocketpp::connection_hdl hdl);
context_ptr on_tls_init();

#endif // WEBSOCKET_CLIENT_EXAMPLE_H
