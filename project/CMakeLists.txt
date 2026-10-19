cmake_minimum_required(VERSION 3.16)
project(UdpGameServer CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(server src/server.cpp src/game.cpp src/comms.cpp)
