cmake_minimum_required(VERSION 3.21)

project(ChatApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)

qt_standard_project_setup()

qt_add_executable(ChatApplication
    Code/GUI/Login_window/main.cpp
    Code/GUI/Login_window/loginwindow.cpp
    Code/GUI/Login_window/loginwindow.h
    Code/GUI/main_window/mainwindow.cpp
    Code/GUI/main_window/mainwindow.h
    Code/GUI/register_window/registerwindow.cpp
    Code/GUI/register_window/registerwindow.h
    Code/GUI/chat_interaction/chat_interaction.cpp
    Code/GUI/chat_interaction/chat_interaction.h
    Code/GUI/Chat_window/ChatWindow.cpp
    Code/GUI/Chat_window/ChatWindow.h
    Code/client/TCPClient.cpp
    Code/client/TCPClient.h
    Code/shared/Protocol.cpp
    Code/shared/Protocol.h
    Code/shared/Message.h
)

target_link_libraries(ChatApplication PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)

qt_add_executable(ChatServer
    Code/server/main.cpp
    Code/server/ChatServer.cpp
    Code/server/ChatServer.h
    Code/server/ClientHandler.cpp
    Code/server/ClientHandler.h
    Code/server/ConnectionManager.cpp
    Code/server/ConnectionManager.h
    Code/database/code/database.cpp
    Code/database/code/database.h
    Code/database/code/user_manager.cpp
    Code/database/code/user_manager.h
    Code/database/code/conversation_manager.cpp
    Code/database/code/conversation_manager.h
    Code/chatServer/MessageManage.cpp
    Code/chatServer/MessageManager.h
    Code/shared/Protocol.cpp
    Code/shared/Protocol.h
    Code/shared/Message.h
)

target_link_libraries(ChatServer PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
)

if(WIN32)
    add_custom_command(TARGET ChatApplication PRE_LINK
        COMMAND cmd /c "taskkill /F /IM ChatApplication.exe >nul 2>&1 & exit /b 0"
    )
    add_custom_command(TARGET ChatServer PRE_LINK
        COMMAND cmd /c "taskkill /F /IM ChatServer.exe >nul 2>&1 & exit /b 0"
    )
endif()

add_dependencies(ChatApplication ChatServer)
