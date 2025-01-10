#include "WebSocket_Client_Example.h"
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;

void Ohlcv::print() const {
    std::cout << "Open: " << open << ", High: " << high << ", Low: " << low
              << ", Close: " << close << ", Volume: " << volume << std::endl;
}

void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    try {
        std::string payload = msg->get_payload();
        // Process the payload, parse JSON or protocol buffers
        std::cout << "Message received: " << payload << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error in on_message: " << e.what() << std::endl;
    }
}

void on_open(client* c, websocketpp::connection_hdl hdl) {
    try {
        std::string subscription_message = R"({"type": "subscribe", "channels": ["ticker"]})";
        c->send(hdl, subscription_message, websocketpp::frame::opcode::text);
        std::cout << "Connection opened and subscription message sent." << std::endl;
    } catch (const websocketpp::exception& e) {
        std::cerr << "Error in on_open: " << e.what() << std::endl;
    }
}

context_ptr on_tls_init() {
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (const std::exception& e) {
        std::cerr << "Error in on_tls_init: " << e.what() << std::endl;
    }
    return ctx;
}
