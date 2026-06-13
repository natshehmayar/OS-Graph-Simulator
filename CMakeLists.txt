cmake_minimum_required(VERSION 4.2)
project(OSFirstProjectStep C)

set(CMAKE_C_STANDARD 11)

add_executable(OSFirstProjectStep dijkstra.c
        cmake-build-debug/gui.cpp
        cmake-build-debug/gui.c
        gui.c
        travelers.c)
