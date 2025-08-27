cmake_minimum_required(VERSION 3.10)
project(FastSignal VERSION 1.0.0 LANGUAGES CXX)

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

# Include dependencies
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/eb2d85edd0bff7a712b6aff147cd9f789f0d7d0b.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nanobench)

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

# Create example executable
add_executable(fastsignal_example fastsignal_example.cpp)
target_link_libraries(fastsignal_example fastsignal)

# Set properties for the example executable
set_target_properties(fastsignal_example PROPERTIES
    OUTPUT_NAME "fastsignal_example"
)

enable_testing()

add_executable(
  fastsignal_test
  fastsignal_test.cpp
)

target_link_libraries(
  fastsignal_test
  fastsignal
  GTest::gtest_main
  GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(fastsignal_test)

add_executable(fastsignal_benchmark fastsignal_benchmark.cpp)
target_link_libraries(fastsignal_benchmark PRIVATE nanobench)
target_compile_options(fastsignal_benchmark PRIVATE -O3 -DNDEBUG)

# Add custom target for running the example
add_custom_target(example
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_example
    DEPENDS fastsignal_example
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running FastSignal example..."
)

# Add custom target for running the tests
add_custom_target(run_tests
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_test
    DEPENDS fastsignal_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal tests..."
)

add_custom_target(run_benchmark
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_benchmark
    DEPENDS fastsignal_benchmark
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal benchmark..."
)
