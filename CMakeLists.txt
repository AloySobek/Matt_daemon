cmake_minimum_required(VERSION 3.16)

project(Matt_daemon)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -Wextra -Werror)

add_executable(${PROJECT_NAME} src/main.cpp src/Matt_daemon.cpp
                               src/Tintin_reporter.cpp)
add_executable(Ben_AFK src/client.cpp src/Tintin_reporter.cpp)
