# Create example executable
add_executable(fastsignal_examples fastsignal_examples.cpp)
target_link_libraries(fastsignal_examples fastsignal)

# Set properties for the example executable
set_target_properties(fastsignal_examples PROPERTIES
    OUTPUT_NAME "fastsignal_examples"
)

# Add custom target for running the example
add_custom_target(examples
    COMMAND ${CMAKE_BINARY_DIR}/bin/fastsignal_examples
    DEPENDS fastsignal_examples
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMENT "Running FastSignal examples..."
)