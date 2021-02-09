cmake_minimum_required(VERSION 3.17)
project(play_1)

#set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD 17)

add_executable(play_1 main.cpp bst.cpp)
add_executable(test test.cpp bst.cpp)
add_executable(bench bench.cpp bst.cpp)