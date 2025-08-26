#include <nanobench.h>
#include <memory>
#include <random>

#include "fastsignal.hpp"

using namespace fastsignal;

volatile uint64_t sink = 0;
void set_global_value1()
{
    sink++;
}

struct ObserverI
{
    virtual void set_global_value1() = 0;
    virtual ~ObserverI() = default;
};

struct Observer1 : public ObserverI
{
    void set_global_value1() override
    {
        sink++;
    }
};

struct Observer2 : public ObserverI
{
    void set_global_value1() override
    {
        sink++;
    }
};

struct ObserverSigI
{
    virtual void set_global_value1() = 0;
    virtual ~ObserverSigI() = default;
};

struct ObserverSig1 : public ObserverSigI
{
    void set_global_value1() override
    {
        sink++;
    }
};

struct ObserverSig2 : public ObserverSigI
{
    void set_global_value1() override
    {
        sink++;
    }
};

struct Subject
{
    FastSignal<void(void)> sig;
    std::vector<ObserverI *> observers;

    void add_observer(ObserverI *observer)
    {
        observers.push_back(observer);
    }

    void notify_observers()
    {
        for (auto observer : observers)
            observer->set_global_value1();
    }

    void notify_sig()
    {
        sig();
    }
};

constexpr int OBSERVERS_COUNT = 2500;
std::vector<std::unique_ptr<ObserverI>> observersOop;
std::vector<std::unique_ptr<ObserverSigI>> observersSig;

void create_observers()
{
    // Fixed seed for reproducible results
    std::mt19937 gen(time(nullptr));
    std::uniform_int_distribution<> dis(1, 100);

    for (int i = 0; i < OBSERVERS_COUNT; ++i) {
        if (dis(gen) & 1) {
            observersOop.emplace_back(std::make_unique<Observer1>());
            observersSig.emplace_back(std::make_unique<ObserverSig1>());
        } else {
            observersOop.emplace_back(std::make_unique<Observer2>());
            observersSig.emplace_back(std::make_unique<ObserverSig2>());
        }
    }
}

void benchmark_add_observer(Subject& subject)
{
    ankerl::nanobench::Bench().run("benchmark_add_observer - observer", [&]() {
        for (int i = 0; i < OBSERVERS_COUNT; ++i)
            subject.add_observer(observersOop[i].get());
    });

    ankerl::nanobench::Bench().run("benchmark_add_observer - fastsignal", [&]() {
        for (int i = 0; i < OBSERVERS_COUNT; ++i)
            subject.sig.add<&ObserverSigI::set_global_value1>(observersSig[i].get());
    });
}

void benchmark_notify_observers(Subject& subject)
{
    ankerl::nanobench::Bench().run("benchmark_notify_observers - observer", [&]() {
        subject.notify_observers();
    });

    ankerl::nanobench::Bench().run("benchmark_notify_observers - fastsignal", [&]() {
        subject.notify_sig();
    });
}


int main()
{
    Subject subject;

    create_observers();

    benchmark_add_observer(subject);
    benchmark_notify_observers(subject);

    return 0;
}
