
cmake_minimum_required(VERSION 3.21)

project(ChatApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Code")

qt_add_executable(ChatApplication
    ${CODE_DIR}/GUI/Chat_window/ChatWindow.cpp
    ${CODE_DIR}/GUI/Chat_window/ChatWindow.h
    ${CODE_DIR}/GUI/Chat_window/ChatWindow.ui
    ${CODE_DIR}/GUI/Login_window/main.cpp
    ${CODE_DIR}/GUI/Login_window/loginwindow.cpp
    ${CODE_DIR}/GUI/Login_window/loginwindow.h

    ${CODE_DIR}/GUI/main_window/mainwindow.cpp
    ${CODE_DIR}/GUI/main_window/mainwindow.h

    ${CODE_DIR}/GUI/register_window/registerwindow.cpp
    ${CODE_DIR}/GUI/register_window/registerwindow.h

    ${CODE_DIR}/GUI/chat_interaction/chat_interaction.cpp
    ${CODE_DIR}/GUI/chat_interaction/chat_interaction.h

    ${CODE_DIR}/client/TCPClient.cpp
    ${CODE_DIR}/client/TCPClient.h

    ${CODE_DIR}/shared/Protocol.cpp
    ${CODE_DIR}/shared/Protocol.h
    ${CODE_DIR}/shared/Message.h
)

target_include_directories(ChatApplication PRIVATE
    ${CODE_DIR}/GUI/Login_window
    ${CODE_DIR}/GUI/main_window
    ${CODE_DIR}/GUI/register_window
    ${CODE_DIR}/GUI/chat_interaction
    ${CODE_DIR}/client
    ${CODE_DIR}/shared
)

target_link_libraries(ChatApplication PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
