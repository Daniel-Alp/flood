cmake_minimum_required(VERSION 3.17)
project(rhubarb)

set(CMAKE_C_STAND 99)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable (rhubarb "src/chunk.c" "src/compiler.c" "src/main.c" "src/memory.c" "src/scanner.c" "src/vm.c")