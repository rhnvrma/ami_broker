#include "WebSocketHandler.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "json.hpp"
#include "MarketDataFeedV3.pb.h"
#include <iostream>
#include <thread>
#include <mutex>

std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;
std::mutex globalMapMutex;

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;


// Helper functions
void Ohlcv::print() const {
    std::cout << "open: " << open << ", high: " << high << ", low: " << low
              << ", close: " << close << ", volume: " << volume << std::endl;
}

void on_message(client *c, websocketpp::connection_hdl hdl, message_ptr msg) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    upstox::FeedResponse demo;
    demo.ParseFromString(msg->get_payload());

    const auto &feedsMap = demo.feeds();
    for (const auto &[key, feed] : feedsMap) {
        if (feed.has_fullfeed() && feed.fullfeed().has_marketff()) {
            const auto &marketFF = feed.fullfeed().marketff();
            const auto &ohlcs = marketFF.marketohlc().ohlc();
            for (const auto &ohlc : ohlcs) {
                if (ohlc.interval() == "I1") {
                    Ohlcv temp = {ohlc.open(), ohlc.high(), ohlc.low(), ohlc.close(), ohlc.vol()};
                    
                    // Thread-safe update
                    std::lock_guard<std::mutex> lock(globalMapMutex);
                    globalMap[key][ohlc.ts()] = temp;

                    temp.print();
                }
            }
        }
    }
}

void on_open(client *c, websocketpp::connection_hdl hdl) {
    json j;
    j["guid"] = "af5a7662-0639-4d7e-be85-a9a222d3c4fd";
    j["method"] = "sub";
    j["data"] = {
        {"instrumentKeys", {"NSE_EQ|INE062A01020"}},
        {"mode", "full"}
    };

    std::string serialized = j.dump();
    std::vector<uint8_t> binary_data(serialized.begin(), serialized.end());

    c->send(hdl, serialized, websocketpp::frame::opcode::binary);
}

static context_ptr on_tls_init() {
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

void startWebSocketClient(const std::string &uri) {
    client c;

    try {
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        c.init_asio();
        c.set_tls_init_handler(bind(&on_tls_init));
        c.set_message_handler(bind(&on_message, &c, ::_1, ::_2));
        c.set_open_handler(bind(&on_open, &c, ::_1));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return;
        }

        c.connect(con);
        c.run();
    } catch (websocketpp::exception const &e) {
        std::cout << e.what() << std::endl;
    }
}
