
cmake_minimum_required(VERSION 3.21)

project(ChatApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(ChatApplication
    main.cpp
    loginwindow.cpp
    loginwindow.h
    mainwindow.cpp
    mainwindow.h
    registerwindow.cpp
    registerwindow.h
)

target_link_libraries(ChatApplication PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

qt_add_executable(ChatServer
    server/main.cpp
    server/ChatServer.cpp
    server/ChatServer.h
    server/ClientHandler.cpp
    server/ClientHandler.h
)

target_link_libraries(ChatServer PRIVATE
    Qt6::Core
    Qt6::Network
)
