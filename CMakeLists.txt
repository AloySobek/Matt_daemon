cmake_minimum_required(VERSION 3.20)

project(Matt_daemon)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(${PROJECT_NAME} src/main.cpp)
