cmake_minimum_required(VERSION 3.16)
project(Lab5 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(WIN32)
    set(SOCKET_LIB ws2_32)
endif()

add_executable(server
    server.cpp
    serial.cpp
    utils.cpp
    sqlite3.c
)

target_include_directories(server PRIVATE .)

if(WIN32)
    target_link_libraries(server PRIVATE ${SOCKET_LIB})
endif()

file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})