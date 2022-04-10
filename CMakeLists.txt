cmake_minimum_required(VERSION 3.16)
project(my_malloc C)

set(CMAKE_C_STANDARD 99)

add_executable(my_malloc mvalloc.c main.c)
