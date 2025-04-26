cmake_minimum_required(VERSION 3.17)
project(rhubarb)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(rhubarb src/debug.c src/error.c src/main.c src/parse.c src/scan.c src/resolve.c src/ty.c util/arena.c util/memory.c)