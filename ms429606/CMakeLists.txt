cmake_minimum_required(VERSION 3.8)
project(MIMUW-FORK C)

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare")

add_library(err err.c)
add_library(readers-writers-template readers-writers-template.c)
add_library(path_utils path_utils.c)
add_library(HashMap HashMap.c)
add_library(Tree Tree.c)
#add_executable(main main.c)
include("${CMAKE_CURRENT_SOURCE_DIR}/testy-zad2/CMakeExtension.txt")
target_link_libraries(main Tree HashMap readers-writers-template err pthread path_utils)

install(TARGETS DESTINATION .)
