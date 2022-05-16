cmake_minimum_required(VERSION 3.10)
project(MyPing)

set(CMAKE_CXX_STANDARD 14)

add_executable(MyPing src/ping.cpp include/ping.h src/loggers.cpp include/loggers.h src/utils.cpp include/utils.h main.cpp)
