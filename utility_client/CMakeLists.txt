cmake_minimum_required(VERSION 3.14)
project(WebSocketExample)

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES
    sample_main.cpp
    # Add other source files here
)
# Add WebSocket++ and Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
find_package(OpenSSL REQUIRED)
# Add Protobuf
find_package(Protobuf REQUIRED)
include_directories(proto_files
    ${Protobuf_INCLUDE_DIRS} 
    ${OPENSSL_INCLUDE_DIR}
)

# Add JSON library
# add_subdirectory(nlohmann_json)
set(PROTO_FILES MarketDataFeedV3.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
add_library(proto_files ${PROTO_SRCS} ${PROTO_HDRS})

# Include directories
include_directories(${CMAKE_SOURCE_DIR} ${OPENSSL_INCLUDE_DIR})

# Add the executable
add_executable(main sample_main.cpp WebSocketHandler.cpp)

# Link libraries
target_link_libraries(main ${Boost_LIBRARIES} ${Protobuf_LIBRARIES}  proto_files ${OPENSSL_LIBRARIES})
