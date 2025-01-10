#include "WebSocketHandler.h"
#include <thread>
#include <iostream>

int main() {
    std::string uri;
    std::cout << "Enter WebSocket URI: ";
    std::cin >> uri;

    // Start WebSocket client in a separate thread
    std::thread wsThread(startWebSocketClient, uri);

    // Main program logic
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(globalMapMutex);
        for (const auto &[key, tsMap] : globalMap) {
            std::cout << "Instrument: " << key << std::endl;
            for (const auto &[timestamp, ohlcv] : tsMap) {
                ohlcv.print();
            }
        }
    }

    wsThread.join();
    return 0;
}
