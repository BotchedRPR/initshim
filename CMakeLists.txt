cmake_minimum_required(VERSION 3.29)
project(initshim C)

# C11
set(CMAKE_C_STANDARD 11)

# Set include directories
include_directories("include")

add_executable(initshim src/main.c)
