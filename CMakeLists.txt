cmake_minimum_required(VERSION 3.25)
project(controls_middleware VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# add libraries
add_library(controls_middleware
    src/server.cpp
)

# add includes
target_include_directories(controls_middleware
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(middleware src/main.cpp)
target_link_libraries(middleware PRIVATE controls_middleware)
