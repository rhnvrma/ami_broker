#include "WebSocketClient.hpp"
#include <iostream>

// Global map to store data
// std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;
// std::mutex WebSocketClient::globalMapMutex;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
static context_ptr on_tls_init() {
    // establishes a SSL connection
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception &e) {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
}

void Ohlcv::print() const {
    std::cout << "open: " << open << ", high: " << high << ", low: " << low
              << ", close: " << close << ", volume: " << volume << std::endl;
}

WebSocketClient::WebSocketClient(const std::string& uri) : uri(uri), running(false) {}

WebSocketClient::~WebSocketClient() {
    stop();
}

void WebSocketClient::start() {
    if (running.load()) return;

    running.store(true);
    client_thread = std::thread(&WebSocketClient::run, this);
}

void WebSocketClient::stop() {
    if (!running.load()) return;

    running.store(false);
    client.stop();
    if (client_thread.joinable()) {
        client_thread.join();
    }
}

void WebSocketClient::run() {
    try {
        client.init_asio();
        client.set_tls_init_handler(bind(&on_tls_init));
        client.set_message_handler(bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
        client.set_open_handler(bind(&WebSocketClient::on_open, this, std::placeholders::_1));

        websocketpp::lib::error_code ec;
        websocket_client::connection_ptr con = client.get_connection(uri, ec);
        if (ec) {
            std::cerr << "Could not create connection: " << ec.message() << std::endl;
            return;
        }

        client.connect(con);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error in WebSocketClient::run: " << e.what() << std::endl;
    }
}

void WebSocketClient::on_message(websocketpp::connection_hdl hdl, websocketpp::config::asio_client::message_type::ptr msg) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    upstox::FeedResponse demo;
    demo.ParseFromString(msg->get_payload());

    const auto& feedsMap = demo.feeds();
    for (const auto& [key, feed] : feedsMap) {
        if (feed.has_fullfeed() && feed.fullfeed().has_marketff()) {
            const auto& marketFF = feed.fullfeed().marketff();
            const auto& ohlcs = marketFF.marketohlc().ohlc();
            for (const auto& ohlc : ohlcs) {
                if (ohlc.interval() == "I1") {
                    Ohlcv temp{ohlc.open(), ohlc.high(), ohlc.low(), ohlc.close(), ohlc.vol()};

                    // Lock the global map for thread-safe access
                    // std::lock_guard<std::mutex> lock(globalMapMutex);
                    globalMap[key][ohlc.ts()] = temp;
                }
            }
        }
    }
}

void WebSocketClient::on_open(websocketpp::connection_hdl hdl) {
    json j;
    j["guid"] = "af5a7662-0639-4d7e-be85-a9a222d3c4fd";
    j["method"] = "sub";
    j["data"] = {
        {"instrumentKeys", {"NSE_EQ|INE062A01020"}},
        {"mode", "full"}
    };

    std::string serialized = j.dump();
    websocketpp::lib::error_code ec;
    client.send(hdl, serialized, websocketpp::frame::opcode::text, ec);

    if (ec) {
        std::cerr << "Error sending subscription message: " << ec.message() << std::endl;
    }
}

std::shared_ptr<boost::asio::ssl::context> WebSocketClient::on_tls_init() {
    auto ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (const std::exception& e) {
        std::cerr << "Error in TLS initialization: " << e.what() << std::endl;
    }
    return ctx;
}
