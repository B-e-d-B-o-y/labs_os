cmake_minimum_required(VERSION 3.16)

project(prog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(prog
    prog.c++
    time_log.c++
    thr.c++
    sem_mng.c++
    mem_shr.c++
)
