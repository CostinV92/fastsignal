cmake_minimum_required(VERSION 3.10)
project(FastSignal VERSION 1.0.0 LANGUAGES CXX)

# Avoid CMake warning about CMP0135
cmake_policy(SET CMP0135 NEW)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Configure debug and release flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address,undefined -fno-omit-frame-pointer")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Create header-only library target
add_library(fastsignal INTERFACE)
target_include_directories(fastsignal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Set properties for the interface library
set_target_properties(fastsignal PROPERTIES
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "A lightweight, header-only C++ signal/slot library"
)

add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(benchmarks)
