#include <iostream>

#include "simplesignal.hpp"

void free_function_no_param()
{
    std::cout << "Hello from free_function_no_param()!\n";
}

void free_function_no_param_example()
{
    std::cout << "Free function without parameters example:\n";
    std::cout << "========================================\n";
    simplesignal::SimpleSignal<void()> sig;
    auto con1 = sig.add(free_function_no_param);

    // Will call free_function_no_param once
    sig();
    con1.disconnect();
    std::cout << "\n";
}

void free_function_with_param(int param)
{
    std::cout << "Hello from test_free_function_with_param(" << param << ")!\n";
}

void free_function_with_param_example()
{
    std::cout << "Free function with parameters example:\n";
    std::cout << "======================================\n";
    simplesignal::SimpleSignal<void(int)> sig;
    auto con1 = sig.add(free_function_with_param);

    // Will call free_function_with_param(1)
    sig(1);
    con1.disconnect();
    std::cout << "\n";
}

struct Param
{
    int member;

    Param(int member) : member(member) {}

    friend std::ostream& operator<<(std::ostream& os, const Param& param)
    {
        os << "Param(" << param.member << ")";
        return os;
    }
};

void free_function_with_complex_param(Param param)
{
    std::cout << "Hello from test_free_function_with_complex_param(" << param << ")!\n";
}

void free_function_with_complex_param_example()
{
    std::cout << "Free function with complex parameters example:\n";
    std::cout << "==============================================\n";
    simplesignal::SimpleSignal<void(Param)> sig;
    auto con1 = sig.add(free_function_with_complex_param);

    Param param{1};
    sig(param);
    con1.disconnect();
    std::cout << "\n";
}

struct Interface {
    virtual void virtual_member_function(Param param) = 0;
    virtual ~Interface() = default;
};

struct Object : public Interface {
    void member_function_with_complex_param(Param param)
    {
        //param.member = 2;
        std::cout << "Hello from Object::member_function_with_complex_param(" << param << ")!\n";
    }

    void virtual_member_function(Param param) override
    {
        std::cout << "Hello from Object::virtual_member_function(" << param << ")!\n";
    }
};

void member_function_with_complex_param_example()
{
    std::cout << "Member function with complex parameters example:\n";
    std::cout << "==============================================\n";
    Object object;
    simplesignal::SimpleSignal<void(Param)> sig;
    sig.add<&Object::member_function_with_complex_param>(&object);
    Param param{1};
    sig(param);
    std::cout << "\n";
}

void virtual_member_function_example()
{
    std::cout << "Member function of interface example:\n";
    std::cout << "====================================\n";
    Interface *object = new Object();
    simplesignal::SimpleSignal<void(Param)> sig;
    sig.add<&Interface::virtual_member_function>(object);
    Param param{1};
    sig(param);
    std::cout << "\n";
    delete object;
}

int main()
{
    free_function_no_param_example();
    free_function_with_param_example();
    free_function_with_complex_param_example();
    member_function_with_complex_param_example();
    virtual_member_function_example();
}
