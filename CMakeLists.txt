cmake_minimum_required(VERSION 3.23)
project(Grid)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2")

add_executable(Grid grid.cpp)