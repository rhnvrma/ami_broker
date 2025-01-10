
/*
 * Copyright (c) 2016, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "json.hpp"
#include "MarketDataFeedV3.pb.h"
#include <iostream>
#include<unordered_map>
using json = nlohmann::json;
struct Ohlcv {
    double open=0;
    double high=0;
    double low=0;
    double close=0;
    int64_t volume=0;
    void print() const {
        std::cout << "open: " << open << ", high: " << high << ", low: " << low<< ", close: " << close << ", volume: "<<volume<<std::endl;
    }
};
std::unordered_map<std::string,std::unordered_map<int64_t, Ohlcv>> globalMap;
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    upstox::FeedResponse demo;
    demo.ParseFromString(msg->get_payload());
    // std::cout << "on_message called with hdl: " << hdl.lock().get()
    //           << " and message: " <<msg->get_opcode()<<msg->get_payload()
    //           << std::endl;
    const auto& feedsMap = demo.feeds();
    for (const auto& [key, feed] : feedsMap) {
        if (feed.has_fullfeed() && feed.fullfeed().has_marketff()) {
            const auto& marketFF = feed.fullfeed().marketff();
            // const auto& ohlcmap = marketFF.marketohlc();
            // std::cout<<ohlcmap.DebugString();
            const auto& ohlcs = marketFF.marketohlc().ohlc();
            for (const auto& ohlc : ohlcs) {
                if (ohlc.interval()=="I1"){
                    Ohlcv temp={ohlc.open(),ohlc.high(),ohlc.low(),ohlc.close(),ohlc.vol()};
                    globalMap[key][ohlc.ts()]=temp;
                    std::cout<<ohlc.ts()<<std::endl;
                    temp.print();

                    
                }
                // std::cout << "OHLC Entry: " << ohlc.interval() << std::endl;
                
            }
                
        }

        }
    // websocketpp::lib::error_code ec;

    // c->send(hdl, msg->get_payload(), msg->get_opcode(), ec);
    // if (ec) {
    //     std::cout << "Echo failed because: " << ec.message() << std::endl;
    // }
}
 void on_open(client * c, websocketpp::connection_hdl hdl) {
        // m_status = "Open";
        websocketpp::lib::error_code ec;
        json j;
        j["guid"] = "af5a7662-0639-4d7e-be85-a9a222d3c4fd";
        j["method"] = "sub";
        j["data"] = {
            {"instrumentKeys", {"NSE_EQ|INE062A01020"}},
            {"mode", "full"}
        };

        // Serialize to a binary format
        std::string serialized = j.dump();
        std::vector<uint8_t> binary_data(serialized.begin(), serialized.end());
        c->send(hdl, binary_data.data(),binary_data.size(), websocketpp::frame::opcode::binary);

        // client::connection_ptr con = c->get_con_from_hdl(hdl);
        // m_server = con->get_response_header("Server");
    }
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

int main(int argc, char* argv[]) {
    
    // Create a client endpoint
    client c;

    std::string uri;
    std::cin>>uri;

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        // Set logging to be pretty verbose (everything except message payloads)
        c.set_access_channels(websocketpp::log::alevel::all);
        c.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize ASIO
        c.init_asio();
        c.set_tls_init_handler(bind(&on_tls_init));

        // Register our message handler
        c.set_message_handler(bind(&on_message,&c,::_1,::_2));
        c.set_open_handler(bind(&on_open,&c,::_1));
        websocketpp::lib::error_code ec;
        client::connection_ptr con = c.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return 0;
        }

        // Note that connect here only requests a connection. No network messages are
        // exchanged until the event loop starts running in the next line.
        c.connect(con);

        // Start the ASIO io_service run loop
        // this will cause a single connection to be made to the server. c.run()
        // will exit when this connection is closed.
        c.run();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}
