
cmake_minimum_required(VERSION 3.21)

project(ChatTCP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

# Duong dan Qt
set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/mingw_64")

# Tim cac module Qt
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Sql
    Network
)

# Cac file source cua du an
add_executable(ChatTCP
    main.cpp

    database.cpp
    database.h

    user_manager.cpp
    user_manager.h

    conversation_manager.cpp
    conversation_manager.h
)

# Link thu vien Qt
target_link_libraries(ChatTCP PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)
