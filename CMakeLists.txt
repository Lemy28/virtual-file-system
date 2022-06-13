cmake_minimum_required(VERSION 3.19)
project(VFS)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(VFS
        MingOS.cpp
        MingOS.h
        system_func.cpp
        user_func.cpp
        rbtree.cpp)
