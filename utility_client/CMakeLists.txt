cmake_minimum_required(VERSION 3.15)
project(ProtobufExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find Protobuf, Boost, OpenSSL, and websocketpp packages
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS random)
find_package(OpenSSL REQUIRED)
find_package(websocketpp REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${Protobuf_INCLUDE_DIRS} ${websocketpp_INCLUDE_DIRS})

# Add the executable
add_executable(${PROJECT_NAME} sample_main.cpp MarketDataFeedV3.pb.cc WebSocketHandler.cpp)

# Link the libraries
target_link_libraries(${PROJECT_NAME} PRIVATE protobuf::libprotobuf ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} websocketpp::websocketpp Boost::random)
