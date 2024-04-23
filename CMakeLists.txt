cmake_minimum_required(VERSION 3.29)
project(initshim C)

# C11
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})

# Set include directories
include_directories("include")
find_package(Libdrm 2.4.116)

add_executable(initshim
        src/main.c
        src/drm.c
        include/drm.h
        include/debug_config.h)
target_link_libraries(initshim drm)
