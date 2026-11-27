
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

qt_add_executable(ChatServer
    ${CODE_DIR}/server/main.cpp
    ${CODE_DIR}/server/ChatServer.cpp
    ${CODE_DIR}/server/ChatServer.h
    ${CODE_DIR}/server/ClientHandler.cpp
    ${CODE_DIR}/server/ClientHandler.h
    ${CODE_DIR}/server/ConnectionManager.cpp
    ${CODE_DIR}/server/ConnectionManager.h
    ${CODE_DIR}/database/code/database.cpp
    ${CODE_DIR}/database/code/database.h
    ${CODE_DIR}/database/code/user_manager.cpp
    ${CODE_DIR}/database/code/user_manager.h
    ${CODE_DIR}/database/code/conversation_manager.cpp
    ${CODE_DIR}/database/code/conversation_manager.h
    ${CODE_DIR}/chatServer/MessageManage.cpp
    ${CODE_DIR}/chatServer/MessageManager.h
    ${CODE_DIR}/shared/Protocol.cpp
    ${CODE_DIR}/shared/Protocol.h
    ${CODE_DIR}/shared/Message.h
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
