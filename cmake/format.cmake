# Format.cmake
#
# Adds `format` and `format-check` targets that run clang-format over all
# tracked C/C++ source and header files in the repository.
#
# Usage (from your top-level CMakeLists.txt):
#   include(cmake/Format.cmake)
#
# Requires: clang-format and git available on PATH.
# If either is missing, the targets are silently skipped (no build failure).

find_program(CLANG_FORMAT_EXE NAMES clang-format)
find_program(GIT_EXE NAMES git)

if(NOT CLANG_FORMAT_EXE)
    message(STATUS "clang-format not found - 'format' / 'format-check' targets disabled")
elseif(NOT GIT_EXE)
    message(STATUS "git not found - 'format' / 'format-check' targets disabled")
else()
    # Get the tracked source/header files in git
    execute_process(
        COMMAND ${GIT_EXE} -C ${CMAKE_SOURCE_DIR} ls-files
        "*.c" "*.cc" "*.cpp" "*.cxx"
        "*.h" "*.hh" "*.hpp" "*.hxx"
        OUTPUT_VARIABLE FORMAT_FILES_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REPLACE "\n" ";" FORMAT_FILES_RELATIVE "${FORMAT_FILES_RAW}")

    # get the absolute paths of the collected files
    set(FORMAT_FILES "")
    foreach(_file ${FORMAT_FILES_RELATIVE})
        if(_file)
            list(APPEND FORMAT_FILES "${CMAKE_SOURCE_DIR}/${_file}")
        endif()    
    endforeach()

    if(NOT FORMAT_FILES)
        message(STATUS "No C/C++ source files found - 'format' / 'format-check' targets disabled")
    else()
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i ${FORMAT_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-format on ${CMAKE_PROJECT_NAME} sources"
            VERBATIM
        )

        add_custom_target(format-check
            COMMAND ${CLANG_FORMAT_EXE} --dry-run -Werror ${FORMAT_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Checking clang-format compliance for ${CMAKE_PROJECT_NAME} sources"
            VERBATIM
        )
    endif()
endif()
    