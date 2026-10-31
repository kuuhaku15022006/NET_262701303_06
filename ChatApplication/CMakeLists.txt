cmake_minimum_required(VERSION 3.21)
project(ClientNetTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CLIENT_DIR "D:/chat-networking/NET_262701303_06/ChatApplication/client")

find_package(Qt6 REQUIRED COMPONENTS Network)

add_executable(ClientNetTest
    net_test.cpp
    ${CLIENT_DIR}/TCPClient.cpp
    ${CLIENT_DIR}/TCPClient.h
)
target_include_directories(ClientNetTest PRIVATE ${CLIENT_DIR})
target_link_libraries(ClientNetTest PRIVATE Qt6::Network)

add_executable(FakeServer fake_server.cpp)
target_link_libraries(FakeServer PRIVATE Qt6::Network)