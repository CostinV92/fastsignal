#include <memory>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>

#include "fastsignal.hpp"
#include "fteng_signals/signals.hpp"

using namespace fastsignal;

class CBench {
    uint32_t id = 0;
    uint32_t iterations = 1;
    std::string name;

    struct Run {
        std::string name;
        std::chrono::duration<double> time;
    };
    std::map<int, Run> runs;

    void print() {

        std::cout << name << "\n";
        for ([[maybe_unused]] auto _ : name)
            std::cout << "-";
        std::cout << "\n";

        auto first_run_duration = runs.begin()->second.time;
        for (auto& run : runs) {
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(run.second.time).count();
            auto relative_duration = duration * 100.0 / std::chrono::duration_cast<std::chrono::nanoseconds>(first_run_duration).count();
            std::cout << run.second.name << ": " << duration << " ns (" << relative_duration << "%)\n";
        }

        std::cout << "============================\n\n";
    }

public:
    CBench(std::string name) : name(name) {}

    void run(std::string test_name, std::function<void()> func) { 
        auto start_time = std::chrono::high_resolution_clock::now();
        for (uint32_t i = 0; i < iterations; ++i)
            func();
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;

        runs[id++] = {test_name, diff};
    }

    CBench& iteration(uint32_t iterations) {
        this->iterations = iterations;
        return *this;
    }

    ~CBench() { print(); }
};

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

    void sig_observers() { sig(); }
    void sig_observers(int value) { sig_int(value); }
    void sig_observers(ComplexParam& param) { sig_cp(param); }

    void fteng_sig_obervers() { fteng_sig(); }
    void fteng_sig_obervers(int value) { fteng_sig_int(value); }
    void fteng_sig_obervers(ComplexParam& param) { fteng_sig_cp(param); }
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

constexpr int ITERATIONS = 1;
constexpr int DIST_COUNT = 100;
constexpr int OBSERVERS_COUNT = 5000;
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

void create_observers(Subject& subject)
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

void bench_call(Subject& subject)
{
    CBench bench("bench_call");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
    for (auto& observer : observers)
            observer->handler1_v();
    });

    bench.run("fastsignal", [&]() {
        subject.sig();
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig();
    });
}

void bench_no_param(Subject& subject)
{
    CBench bench("bench_no_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers();
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers();
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig_obervers();
    });
}

void bench_param(Subject& subject)
{
    CBench bench("bench_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers(0.005);
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers(0.005);
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig_obervers(0.005);
    });
}

void bench_complex_param(Subject& subject)
{
    CBench bench("bench_complex_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers(complex_param);
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers(complex_param);
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig_obervers(complex_param);
    });
}

int main()
{
    Subject subject;
    create_observers(subject);

    bench_call(subject);
    bench_no_param(subject);
    bench_param(subject);
    bench_complex_param(subject);

    return 0;
}