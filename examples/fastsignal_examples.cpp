#include <iostream>

#include "fastsignal.hpp"

void free_function_no_param()
{
    std::cout << "Hello from free_function_no_param()!\n";
}

void free_function_no_param_example()
{
    std::cout << "Free function without parameters example:\n";
    std::cout << "========================================\n";
    fastsignal::FastSignal<void()> sig;
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
    fastsignal::FastSignal<void(int)> sig;
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
    fastsignal::FastSignal<void(Param)> sig;
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
    fastsignal::FastSignal<void(Param)> sig;
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
    fastsignal::FastSignal<void(Param)> sig;
    sig.add<&Interface::virtual_member_function>(object);
    Param param{1};
    sig(param);
    std::cout << "\n";
    delete object;
}

struct Subject
{
    fastsignal::FastSignal<void()> sig;

    void subject_changed() { sig(); }
};

struct ObserverManual
{
    int id;
    fastsignal::ConnectionView conn;

    ObserverManual(int id) : id(id) {}
    ~ObserverManual() { conn.disconnect(); }

    void connect(Subject& subject) { conn = subject.sig.add<&ObserverManual::on_subject_changed>(this); }
    void on_subject_changed() { std::cout << "ObserverManual(" << id << "): on_subject_changed()\n"; }
};

// ObserverAutomatic inherits from fastsignal::Disconnectable
struct ObserverAutomatic : public fastsignal::Disconnectable
{
    int id;
    // ObserverAutomatic doesn't have a destructor
    ObserverAutomatic(int id) : id(id) {}

    // The ConnectionView returned by add() is not stored anywhere
    void connect(Subject& subject) { subject.sig.add<&ObserverAutomatic::on_subject_changed>(this); }
    void on_subject_changed() { std::cout << "ObserverAutomatic(" << id << "): on_subject_changed()\n"; }
};

void observer_manual_management_example()
{
    std::cout << "Observer manual management example:\n";
    std::cout << "====================================\n";

    Subject subject;
    {
        ObserverManual observer1(1);
        ObserverManual observer2(2);

        observer1.connect(subject);
        observer2.connect(subject);

        std::cout << "Subject changed\n";
        // Will print:
        // Observer(1): on_subject_changed()
        // Observer(2): on_subject_changed()
        subject.subject_changed();
    }
    // ObserverManual objects are destroyed here
    // They are disconnected from the signal by the destructor
    std::cout << "ObserverManual objects are destroyed here\n";
    std::cout << "Subject changed\n";
    // Will print nothing
    subject.subject_changed();
    std::cout << "\n";
}

void observer_automatic_management_example()
{
    std::cout << "Observer automatic management example:\n";
    std::cout << "====================================\n";
    Subject subject;
    {
        ObserverAutomatic observer1(1);
        ObserverAutomatic observer2(2);

        observer1.connect(subject);
        observer2.connect(subject);

        std::cout << "Subject changed\n";
        // Will print:
        // ObserverAutomatic(1): on_subject_changed()
        // ObserverAutomatic(2): on_subject_changed()
        subject.subject_changed();
    }
    // ObserverAutomatic objects are destroyed here
    // They are disconnected from the signal automatically
    std::cout << "ObserverAutomatic objects are destroyed here\n";
    std::cout << "Subject changed\n";
    // Will print nothing
    subject.subject_changed();
    std::cout << "\n";
}

int main()
{
    free_function_no_param_example();
    free_function_with_param_example();
    free_function_with_complex_param_example();
    member_function_with_complex_param_example();
    virtual_member_function_example();

    observer_manual_management_example();
    observer_automatic_management_example();
}
