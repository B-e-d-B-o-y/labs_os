cmake_minimum_required(VERSION 3.16)

project(Library LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(library)

set(EXAMPLE_NAME test)
add_executable(${EXAMPLE_NAME} test.c++)

target_link_libraries(${EXAMPLE_NAME} PRIVATE library)