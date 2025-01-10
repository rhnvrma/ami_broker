#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <unordered_map>
#include <string>
#include <mutex>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
struct Ohlcv {
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int64_t volume = 0;

    void print() const;
};

// Declare the global dictionary and mutex
extern std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;
extern std::mutex globalMapMutex;

// Function to start the WebSocket client
void startWebSocketClient(const std::string &uri);

#endif // WEBSOCKET_CLIENT_H
