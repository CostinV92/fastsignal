#include <benchmark/benchmark.h>
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

volatile uint64_t sink = 0;

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
    FastSignal<void(int)> sig_int;
    FastSignal<void(ComplexParam&)> sig_cp;

    fteng::signal<void()> fteng_sig;
    fteng::signal<void(int)> fteng_sig_int;
    fteng::signal<void(ComplexParam&)> fteng_sig_cp;

    std::vector<ObserverI*> observers;

    void add_observer(ObserverI* observer) { observers.push_back(observer); }

    void notify_observers()
    {
        for (auto observer : observers)
            observer->handler1_v();
    }

    void notify_observers(int value)
    {
        for (auto observer : observers)
            observer->handler2_v(value);
    }

    void notify_observers(ComplexParam& param)
    {
        for (auto observer : observers)
            observer->handler3_v(param);
    }

    void sig_obervers() { sig(); }
    void sig_obervers(int value) { sig_int(value); }
    void sig_obervers(ComplexParam& param) { sig_cp(param); }

    void fteng_sig_observers() { fteng_sig(); }
    void fteng_sig_observers(int value) { fteng_sig_int(value); }
    void fteng_sig_observers(ComplexParam& param) { fteng_sig_cp(param); }
};

template<size_t I>
struct Observer : public ObserverI
{
    void handler1() { sink++; }
    void handler1_v() override { sink++; }

    void handler2(double value) { sink += value; }
    void handler2_v(double value) override { sink += value; }

    void handler3(ComplexParam& param) { ++param.value; }
    void handler3_v(ComplexParam& param) override { ++param.value; }

    void connect(Subject& subject)
    {
        subject.sig.add<&Observer::handler1>(this);
        subject.sig_int.add<&Observer::handler2>(this);
        subject.sig_cp.add<&Observer::handler3>(this);

        subject.fteng_sig.connect<&Observer::handler1>(this);
        subject.fteng_sig_int.connect<&Observer::handler2>(this);
        subject.fteng_sig_cp.connect<&Observer::handler3>(this);
    }

    void connect_v(Subject& subject)
    {
        subject.sig.add<&Observer::handler1_v>(this);
        subject.sig_int.add<&Observer::handler2_v>(this);
        subject.sig_cp.add<&Observer::handler3_v>(this);

        subject.fteng_sig.connect<&Observer::handler1_v>(this);
        subject.fteng_sig_int.connect<&Observer::handler2_v>(this);
        subject.fteng_sig_cp.connect<&Observer::handler3_v>(this);
    }
};

constexpr int DIST_COUNT = 100;
constexpr int OBSERVERS_COUNT = 5000;
std::vector<std::unique_ptr<ObserverI>> observers;
Subject subject;

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

void create_observers([[maybe_unused]] const benchmark::State& state)
{
    static bool created = false;

    if (created)
        return;

    created = true;
    // Fixed seed for reproducible results
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

static void BM_observer_call(benchmark::State& state)
{
    for (auto _ : state) {
        for (auto &observer : observers)
            observer->handler1_v();
    }
}
BENCHMARK(BM_observer_call)->Setup(create_observers)->Name("observers_call");

static void BM_sig_call(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig();
    }
}
BENCHMARK(BM_sig_call)->Setup(create_observers)->Name("sig_call");

static void BM_fteng_sig_call(benchmark::State& state)
{

    for (auto _ : state) {
        subject.fteng_sig();
    }
}
BENCHMARK(BM_fteng_sig_call)->Setup(create_observers)->Name("fteng_sig_call");

static void BM_notify_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers();
    }
}
BENCHMARK(BM_notify_observers)->Setup(create_observers)->Name("notify_observers()");

static void BM_sig_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_obervers();
    }
}
BENCHMARK(BM_sig_observers)->Setup(create_observers)->Name("sig_observers()");

static void BM_fteng_sig_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers();
    }
}
BENCHMARK(BM_fteng_sig_observers)->Setup(create_observers)->Name("fteng_sig_observers()");

static void BM_notify_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers(0.005f);
    }
}
BENCHMARK(BM_notify_observers2)->Setup(create_observers)->Name("notify_observers(double)");

static void BM_sig_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_obervers(0.005f);
    }
}
BENCHMARK(BM_sig_observers2)->Setup(create_observers)->Name("sig_observers(double)");

static void BM_fteng_sig_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers(0.005f);
    }
}
BENCHMARK(BM_fteng_sig_observers2)->Setup(create_observers)->Name("fteng_sig_observers(double)");

static void BM_notify_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers(complex_param);
    }
}
BENCHMARK(BM_notify_observers3)->Setup(create_observers)->Name("notify_observers(complex_param)");

static void BM_sig_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_obervers(complex_param);
    }
}
BENCHMARK(BM_sig_observers3)->Setup(create_observers)->Name("sig_observers(complex_param)");

static void BM_fteng_sig_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers(complex_param);
    }
}
BENCHMARK(BM_fteng_sig_observers3)->Setup(create_observers)->Name("fteng_sig_observers(complex_param)");
