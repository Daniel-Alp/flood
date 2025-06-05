cmake_minimum_required(VERSION 3.17)
project(flood)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

add_executable(flood "src/arena.c" "src/chunk.c" "src/compile.c" "src/debug.c" "src/error.c" "src/gc.c" "src/main.c" "src/memory.c" "src/object.c" "src/parse.c" "src/scan.c" "src/sema.c" "src/symbol.c" "src/value.c" "src/vm.c")