#ifndef WEBSOCKETCLIENT_HPP
#define WEBSOCKETCLIENT_HPP

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <unordered_map>
#include <string>
#include <mutex>
#include <thread>
#include "MarketDataFeedV3.pb.h"
#include "json.hpp"

using json = nlohmann::json;

// Structure to hold OHLCV data
struct Ohlcv {
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int64_t volume = 0;
    void print() const;
};

// Define WebSocket client type
typedef websocketpp::client<websocketpp::config::asio_tls_client> websocket_client;

class WebSocketClient {
public:
    WebSocketClient(const std::string& uri);
    ~WebSocketClient();

    void start(); // Start the WebSocket client in a thread
    void stop();  // Stop the WebSocket client

private:
    void on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    static std::shared_ptr<boost::asio::ssl::context> on_tls_init();

    void run();

    std::string uri;
    websocket_client client;
    std::thread client_thread;
    std::atomic<bool> running;

    // Synchronize access to the global map
    // static std::mutex globalMapMutex;
};

#endif // WEBSOCKETCLIENT_HPP
