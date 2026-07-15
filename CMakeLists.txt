cmake_minimum_required(VERSION 3.25)
project(
  controls_middleware
  VERSION 0.1.0
  LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# External Libraries flatbuffers
set(FLATBUFFERS_BUILD_TESTS
    OFF
    CACHE INTERNAL "")
set(FLATBUFFERS_BUILD_FLATC
    ON
    CACHE INTERNAL "")

set(FLATBUFFERS_SRC_DIR "${CMAKE_SOURCE_DIR}/flatbuffers")
add_subdirectory(${FLATBUFFERS_SRC_DIR}
                 ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build EXCLUDE_FROM_ALL)

# flatbuffers schema generation
include(${FLATBUFFERS_SRC_DIR}/CMake/BuildFlatBuffers.cmake)
set(SCHEMA_FILES "${CMAKE_SOURCE_DIR}/schema/telemetry.fbs")
flatbuffers_generate_headers(TARGET telemetry_schema_headers SCHEMAS
                             ${SCHEMA_FILES})

# add a common library
add_library(controls_common src/logging.cpp)
target_include_directories(controls_common PUBLIC "${CMAKE_SOURCE_DIR}/include")

# broker server executable
add_executable(broker_server src/server/server.cpp src/server/main.cpp)

target_link_libraries(broker_server PRIVATE controls_common flatbuffers
                                            telemetry_schema_headers)

# sensor client executable
add_executable(sensor_client src/client/client.cpp src/client/main.cpp)

target_link_libraries(sensor_client PRIVATE controls_common flatbuffers
                                            telemetry_schema_headers)

# include custom targets
include(cmake/format.cmake)
