cmake_minimum_required(VERSION 3.16)
project(OS_Lab4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(emulator
    emulator.cpp
    serial.cpp
    utils.cpp
)

add_executable(logger
    logger.cpp
    serial.cpp
    utils.cpp
)