include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG main
)

FetchContent_Declare(
    nanobench
    GIT_REPOSITORY https://github.com/martinus/nanobench.git
    GIT_TAG v4.1.0
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(
    googlebenchmark
    nanobench
)

add_executable(fastsignal_nbench fastsignal_nbench.cpp)
target_link_libraries(fastsignal_nbench PRIVATE nanobench fastsignal)
target_compile_options(fastsignal_nbench PRIVATE -O3 -DNDEBUG)

add_executable(fastsignal_gbench fastsignal_gbench.cpp)
target_link_libraries(fastsignal_gbench PRIVATE benchmark::benchmark benchmark_main fastsignal)
target_compile_options(fastsignal_gbench PRIVATE -O3 -DNDEBUG)

add_executable(fastsignal_cbench fastsignal_cbench.cpp)
target_link_libraries(fastsignal_cbench PRIVATE fastsignal)
target_compile_options(fastsignal_cbench PRIVATE -O3 -DNDEBUG)

# Add custom targets for running the benchmarks
add_custom_target(nbench
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_nbench
    DEPENDS fastsignal_nbench
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal nanobench benchmark..."
)

add_custom_target(gbench
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_gbench
    DEPENDS fastsignal_gbench
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal Google benchmark..."
)

add_custom_target(cbench
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_cbench
    DEPENDS fastsignal_cbench
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal custom benchmark..."
)

