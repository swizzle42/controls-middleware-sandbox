cmake_minimum_required(VERSION 3.25)
project(IndustrialMiddleware VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_executable(middleware_server
    src/main.cpp
)

target_include_directories(middleware_server PRIVATE src)