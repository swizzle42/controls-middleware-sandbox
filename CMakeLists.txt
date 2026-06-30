cmake_minimum_required(VERSION 3.25)
project(controls_middleware VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# add includes
include_directories(include)

# broker server executable
add_executable(broker_server 
    src/server/server.cpp
    src/server/main.cpp
)

# sensor client executable
add_executable(sensor_client 
    src/client/client.cpp
    src/client/main.cpp
)

# include custom targets
include(cmake/format.cmake)