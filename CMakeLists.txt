cmake_minimum_required(VERSION 3.17)
project(rhubarb)

set(CMAKE_C_STAND 99)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(rhubarb src/chunk.c src/compiler.c src/debug.c src/error.c src/main.c src/parser.c src/scanner.c src/vm.c util/arena.c util/memory.c)