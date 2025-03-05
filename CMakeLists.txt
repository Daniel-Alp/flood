cmake_minimum_required(VERSION 3.17)
project(rhubarb)

set(CMAKE_C_STAND 99)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable (rhubarb "src/main.c" "src/scanner.h" "src/scanner.c")