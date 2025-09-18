# FastSignal

A lightweight, header-only C++ signal/slot library designed for high performance and ease of use.

## Features

- **Header-only**: No compilation required, just include `fastsignal.hpp`
- **Zero dependencies**: Only uses standard C++17 features
- **High performance**: Optimized for minimal overhead during signal emission

## Quick Start

```cpp
#include "fastsignal.hpp"

// Create a signal
fastsignal::FastSignal<void(int)> signal;

// Connect a free function
signal.add([](int value) {
    std::cout << "Received: " << value << std::endl;
});

// Emit the signal
signal(42);  // Output: "Received: 42"
```

## Installation

Since FastSignal is header-only, simply copy `fastsignal.hpp` to your project and include it:

```cpp
#include "fastsignal.hpp"
```

### CMake Integration

If you're using CMake, you can add FastSignal as a subdirectory:

```cmake
add_subdirectory(fastsignal)
target_link_libraries(your_target fastsignal)
```

## Usage Examples

See `examples/` for details.

## Connection Management

### Manual Disconnection

```cpp
fastsignal::FastSignal<void(int)> signal;
auto connection = signal.add([](int value) {
    std::cout << "Connected: " << value << std::endl;
});

signal(1);  // Output: "Connected: 1"
connection.disconnect();
signal(2);  // No output
```

### Automatic Disconnection

For automatic cleanup when objects are destroyed, inherit from `Disconnectable`:

```cpp
class MyObserver : public fastsignal::Disconnectable {
public:
    void handle_event(int value) {
        std::cout << "Observer: " << value << std::endl;
    }
};

fastsignal::FastSignal<void(int)> signal;
MyObserver observer;
signal.add<&MyObserver::handle_event>(&observer);
// Connection automatically disconnects when observer is destroyed
```

## Tests

`googletest` (https://github.com/google/googletest) library is used for UTs.

UTs are available in the `tests/` directory. Run them with:

```bash
mkdir build && cd build
cmake .. -GNinja    # this will download: googletest, nanobench, google benchmark and sbench
ninja test          # this will build: googletest
```

## Benchmarks

There are 3 micro-benchmark libraries that are used for benchmarking `fastsignal`:
- `nanobench` (nbench): https://github.com/martinus/nanobench
- `google benchmark` (gbench): https://github.com/google/benchmark
- `sbench` (cbench): https://github.com/CostinV92/SBench

Performance benchmarks are available in the `benchmarks/` directory. Run them with:

```bash
mkdir build && cd build
cmake .. -GNinja                # this will download: googletest, nanobench, google benchmark and sbench
ninja (nbench|gbench|sbench)    # this will build: (nanobench|google benchmark|sbench)
```
