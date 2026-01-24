cmake_minimum_required(VERSION 3.17)
project(flood)

set(CMAKE_CXX_STANDARD 20)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_executable(flood src/arena.cc src/ast.cc src/chunk.cc src/compile.cc src/debug.cc src/error.cc src/foreign.cc
    src/gc.cc src/main.cc src/parse.cc src/scan.cc src/sema.cc src/value.cc src/vm.cc foreign/listobj_foreign.cc foreign/stringobj_foreign.cc)

target_compile_options(flood PRIVATE
    -Wall -Wextra
)

target_link_libraries(flood PRIVATE m)
