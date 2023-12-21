cmake_minimum_required(VERSION 3.24)
project(PaintTheMost_Game)

set(CMAKE_CXX_STANDARD 14)

include_directories(src/client)
include_directories(src/server)

add_executable(PaintTheMost_Game
        src/client/client.cpp
        src/client/client.h
        src/server/server.cpp
        src/server/server.h)
