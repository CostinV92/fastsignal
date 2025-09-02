include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/eb2d85edd0bff7a712b6aff147cd9f789f0d7d0b.zip
)

FetchContent_MakeAvailable(
    googletest
)

add_executable(
  fastsignal_tests
  fastsignal_tests.cpp
)

target_link_libraries(
  fastsignal_tests
  fastsignal
  GTest::gtest_main
  GTest::gmock_main
)

# Add custom target for running the tests
add_custom_target(test
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_tests
    DEPENDS fastsignal_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running FastSignal tests..."
)