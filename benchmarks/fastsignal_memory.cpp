#include <iostream>

#include "fastsignal.hpp"

using namespace fastsignal;

int main()
{
    std::cout << "FastSignal: " << sizeof(FastSignal<void(int)>) << '\n';
    std::cout << "Callback: " << sizeof(internal::Callback) << '\n';
    std::cout << "Connection: " << sizeof(internal::Connection) << '\n';
    std::cout << "ConnectionView: " << sizeof(ConnectionView) << '\n';
    std::cout << "Disconnectable: " << sizeof(Disconnectable) << "\n\n";

    std::cout << "Every N adds alloc [1+log2(n)](adding to vector) + N(allocating connection)" << '\n';
    std::cout << "Every N disconnectable adds alloc 2 * [1+log2(n)](2x adding to vector) + N(allocating connection)" << "\n\n";

    std::cout << "1 FastSignal add = 1 Callback + 1 Connection (+ 1 ConnectionView) = ";
    std::cout << sizeof(internal::Callback) + sizeof(internal::Connection);
    std::cout << "(" << sizeof(internal::Callback) + sizeof(internal::Connection) + sizeof(ConnectionView) << ")" << '\n';

    std::cout << "1 FastSignal disconnectable add = 1 Callback + 1 Connection + 1 Connection* (+ 1 ConnectionView) = ";
    std::cout << sizeof(internal::Callback) + sizeof(internal::Connection) + sizeof(internal::Connection*);
    std::cout << "(" << sizeof(internal::Callback) + sizeof(internal::Connection) + sizeof(internal::Connection*) + sizeof(ConnectionView) << ")" << '\n';
}
