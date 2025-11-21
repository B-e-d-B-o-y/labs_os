cmake_minimum_required(VERSION 3.16)
project(myprog LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(app
        myprog.c)

target_compile_options(app PRIVATE -Wall -Wextra -pedantic)

set_target_properties(app PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
