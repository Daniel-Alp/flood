cmake_minimum_required(VERSION 3.17)
project(flood)

set(CMAKE_C_STANDARD 99)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_executable(flood src/arena.c src/chunk.c src/compile.c src/debug.c src/error.c src/foreign.c src/gc.c src/main.c src/memory.c src/object.c src/parse.c src/scan.c src/sema.c src/symbol.c src/value.c src/vm.c)

target_compile_options(flood PRIVATE
    -Wall -Wextra
    -Werror=discarded-qualifiers
    -Werror=discarded-array-qualifiers
)

target_link_libraries(flood PRIVATE m)
