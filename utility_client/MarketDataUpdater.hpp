#ifndef MARKET_DATA_UPDATER_H
#define MARKET_DATA_UPDATER_H

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "json.hpp"
#include "MarketDataFeedV3.pb.h"
#include <iostream>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>

using json = nlohmann::json;

struct Ohlcv {
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int64_t volume = 0;

    void print() const {
        std::cout << "open: " << open << ", high: " << high << ", low: " << low
                  << ", close: " << close << ", volume: " << volume << std::endl;
    }
};

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
class MarketDataUpdater {
public:
    using client = websocketpp::client<websocketpp::config::asio_tls_client>;
    using context_ptr = std::shared_ptr<boost::asio::ssl::context>;
    using message_ptr=websocketpp::config::asio_client::message_type::ptr ;

    MarketDataUpdater(const std::string& uri) : uri(uri), running(false) {}

    ~MarketDataUpdater() {
        stop();
    }

    void start() {
        running = true;
        updateThread = std::thread(&MarketDataUpdater::run, this);
    }

    void stop() {
        if (running) {
            running = false;
            if (updateThread.joinable()) {
                updateThread.join();
            }
        }
    }

    const std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>>& getGlobalMap() {
        std::lock_guard<std::mutex> lock(mapMutex);
        return globalMap;
    }

private:
    std::string uri;
    std::atomic<bool> running;
    std::thread updateThread;
    std::mutex mapMutex;
    std::unordered_map<std::string, std::unordered_map<int64_t, Ohlcv>> globalMap;

    static context_ptr on_tls_init() {
        context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
        } catch (const std::exception& e) {
            std::cerr << "Error in context pointer: " << e.what() << std::endl;
        }
        return ctx;
    }

    void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
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
                        Ohlcv temp = {ohlc.open(), ohlc.high(), ohlc.low(), ohlc.close(), ohlc.vol()};
                        std::lock_guard<std::mutex> lock(mapMutex);
                        globalMap[key][ohlc.ts()] = temp;
                        temp.print();
                    }
                }
            }
        }
    }

    void on_open(client* c, websocketpp::connection_hdl hdl) {
        json j;
        j["guid"] = "af5a7662-0639-4d7e-be85-a9a222d3c4fd";
        j["method"] = "sub";
        j["data"] = {
            {"instrumentKeys", {"NSE_EQ|INE062A01020"}},
            {"mode", "full"}
        };

        std::string serialized = j.dump();
        std::vector<uint8_t> binary_data(serialized.begin(), serialized.end());
        c->send(hdl, binary_data.data(), binary_data.size(), websocketpp::frame::opcode::binary);
    }

    void run() {
        client c;

        try {
            c.set_access_channels(websocketpp::log::alevel::none);
            c.clear_access_channels(websocketpp::log::alevel::frame_payload);

            c.init_asio();
            c.set_tls_init_handler(bind(&MarketDataUpdater::on_tls_init, this));

            c.set_message_handler(bind(&MarketDataUpdater::on_message, this, &c, ::_1, ::_2));

            c.set_open_handler(std::bind(&MarketDataUpdater::on_open,this &c,::_1));

            websocketpp::lib::error_code ec;
            auto con = c.get_connection(uri, ec);
            if (ec) {
                std::cerr << "Could not create connection: " << ec.message() << std::endl;
                return;
            }

            c.connect(con);
            c.run();
        } catch (const websocketpp::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
};

#endif // MARKET_DATA_UPDATER_H
