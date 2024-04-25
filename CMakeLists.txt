cmake_minimum_required(VERSION 3.25)
project(initshim C)

# C11
set(CMAKE_C_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})

# Static build
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# Set include directories
include_directories("include")
find_package(Libdrm 2.4.116)

add_executable(initshim
        src/main.c
        src/drm.c
        include/drm.h
)
target_link_libraries(initshim drm)
