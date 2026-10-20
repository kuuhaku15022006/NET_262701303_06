cmake_minimum_required(VERSION 3.16)
project(ChatClientNetwork VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(client_network_test
    src/main.cpp
    src/network/tcp_client.h
    src/network/tcp_client.cpp
)

target_include_directories(client_network_test PRIVATE src)
target_link_libraries(client_network_test PRIVATE Qt6::Core Qt6::Network)