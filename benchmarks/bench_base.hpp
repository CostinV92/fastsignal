#pragma once

#include <memory>
#include <random>
#include <algorithm>

#include "fastsignal.hpp"
#include <signals.hpp>

using namespace fastsignal;

struct ComplexParam
{
    volatile int value;
    std::string str;

    ComplexParam(int value, std::string str) : value(value), str(str) {}
};
ComplexParam complex_param{0, "ComplexParam"};

struct Subject;

struct ObserverI
{
    virtual void handler1_v() = 0;
    virtual void handler2_v(double value) = 0;
    virtual void handler3_v(ComplexParam& param) = 0;
    virtual void connect(Subject& subject) = 0;
    virtual ~ObserverI() = default;
};

struct Subject
{
    FastSignal<void()> sig;
    FastSignal<void(double)> sig_double;
    FastSignal<void(ComplexParam&)> sig_cp;

    fteng::signal<void()> fteng_sig;
    fteng::signal<void(double)> fteng_sig_double;
    fteng::signal<void(ComplexParam&)> fteng_sig_cp;

    std::vector<ObserverI*> observers;

    void add_observer(ObserverI* observer) { observers.push_back(observer); }

    void notify_observers()
    {
        for (auto observer : observers)
            observer->handler1_v();
    }

    void notify_observers(double value)
    {
        for (auto observer : observers)
            observer->handler2_v(value);
    }

    void notify_observers(ComplexParam& param)
    {
        for (auto observer : observers)
            observer->handler3_v(param);
    }

    void sig_observers() { sig(); }
    void sig_observers(double value) { sig_double(value); }
    void sig_observers(ComplexParam& param) { sig_cp(param); }

    void fteng_sig_observers() { fteng_sig(); }
    void fteng_sig_observers(double value) { fteng_sig_double(value); }
    void fteng_sig_observers(ComplexParam& param) { fteng_sig_cp(param); }
};

template<size_t I>
struct Observer : public ObserverI
{
    volatile double sink = 0;

    void handler1() { sink++; }
    void handler1_v() override { sink++; }

    void handler2(double value) { sink += value; }
    void handler2_v(double value) override { sink += value; }

    void handler3(ComplexParam& param) { ++param.value; }
    void handler3_v(ComplexParam& param) override { ++param.value; }

    void connect(Subject& subject)
    {
        subject.sig.add<&Observer::handler1>(this);
        subject.sig_double.add<&Observer::handler2>(this);
        subject.sig_cp.add<&Observer::handler3>(this);

        subject.fteng_sig.connect<&Observer::handler1>(this);
        subject.fteng_sig_double.connect<&Observer::handler2>(this);
        subject.fteng_sig_cp.connect<&Observer::handler3>(this);
    }

    void connect_v(Subject& subject)
    {
        subject.sig.add<&Observer::handler1_v>(this);
        subject.sig_double.add<&Observer::handler2_v>(this);
        subject.sig_cp.add<&Observer::handler3_v>(this);

        subject.fteng_sig.connect<&Observer::handler1_v>(this);
        subject.fteng_sig_double.connect<&Observer::handler2_v>(this);
        subject.fteng_sig_cp.connect<&Observer::handler3_v>(this);
    }
};

constexpr int ITERATIONS = 1;
constexpr int DIST_COUNT = 100;
constexpr int OBSERVERS_COUNT = 5000;

Subject subject;
std::vector<std::unique_ptr<ObserverI>> observers;

template<size_t I>
std::function<std::unique_ptr<ObserverI>(void)> get_observer()
{
    return []() { return std::make_unique<Observer<I>>(); };
}

template<size_t ... Is>
auto getFactoriesImpl(std::index_sequence<Is...>)
{
	return std::array<std::function<std::unique_ptr<ObserverI>(void)>, sizeof...(Is)> { get_observer<Is>()... };
}

auto getFactories()
{
	return getFactoriesImpl(std::make_index_sequence<DIST_COUNT>());
}

static auto fac = getFactories();

void create_observers()
{
    std::mt19937 gen(time(nullptr));
    std::uniform_int_distribution<> dis(0, DIST_COUNT - 1);

    for (int i = 0; i < OBSERVERS_COUNT; ++i) {
        observers.emplace_back(fac[dis(gen)]());
    }

    std::shuffle(observers.begin(), observers.end(), gen);

    for (auto& observer : observers) {
        subject.add_observer(observer.get());
        observer->connect(subject);
    }
}
