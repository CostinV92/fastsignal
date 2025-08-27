#include <nanobench.h>
#include <memory>
#include <random>

#include "fastsignal.hpp"
#include "fteng_signals/signals.hpp"

using namespace fastsignal;
using namespace fteng;

volatile uint64_t sink = 0;
void set_global_value1()
{
    sink++;
}

struct ComplexParam
{
    int value;
    std::string str;
    std::vector<int> vec(std::make_index_sequence<1000>());

    ComplexParam(int value, std::string str) : value(value), str(str) {}
};

ComplexParam complex_param{0, "ComplexParam"};

struct ObserverI
{
    virtual void set_global_value1() = 0;
    virtual void set_global_value2(int value) = 0;
    virtual void set_global_value3(ComplexParam& param) = 0;
    virtual ~ObserverI() = default;
};

template<size_t I>
struct Observer : public ObserverI
{
    void set_global_value1() override
    {
        sink++;
    }

    void set_global_value2(int value) override
    {
        sink += value;
    }

    void set_global_value3(ComplexParam& param) override
    {
        (void)param;
    }
};

struct SubjectWithoutParam
{
    FastSignal<void(void)> sig;
    signal<void(void)> fteng_sig;
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

    void notify_sig_m()
    {
        fteng_sig();
    }
};

struct SubjectWithParam
{
    FastSignal<void(int)> sig;
    signal<void(int)> fteng_sig;
    std::vector<ObserverI *> observers;

    void add_observer(ObserverI *observer)
    {
        observers.push_back(observer);
    }

    void notify_observers(int value)
    {
        for (auto observer : observers)
            observer->set_global_value2(value);
    }

    void notify_sig(int value)
    {
        sig(value);
    }

    void notify_sig_m(int value)
    {
        fteng_sig(value);
    }
};

struct SubjectWithComplexParam
{
    FastSignal<void(ComplexParam&)> sig;
    signal<void(ComplexParam&)> fteng_sig;
    std::vector<ObserverI *> observers;

    void add_observer(ObserverI *observer)
    {
        observers.push_back(observer);
    }

    void notify_observers(ComplexParam& param)
    {
        for (auto observer : observers)
            observer->set_global_value3(param);
    }

    void notify_sig(ComplexParam& param)
    {
        sig(param);
    }

    void notify_sig_m(ComplexParam& param)
    {
        fteng_sig(param);
    }
};

constexpr int ITERATIONS = 1000;
constexpr int DIST_COUNT = 100;
constexpr int OBSERVERS_COUNT = 500;
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
    // Fixed seed for reproducible results
    std::mt19937 gen(time(nullptr));
    std::uniform_int_distribution<> dis(0, DIST_COUNT - 1);

    for (int i = 0; i < OBSERVERS_COUNT; ++i) {
        observers.emplace_back(fac[dis(gen)]());
    }
}

void benchmark_add_observer(SubjectWithoutParam& subject, SubjectWithParam& subject_with_param, SubjectWithComplexParam& subject_with_complex_param)
{
    ankerl::nanobench::Bench b;
    b.title("benchmark_add_observer").relative(true).performanceCounters(true);
    b.run("observer", [&]() {
        for (int i = 0; i < OBSERVERS_COUNT; ++i) {
            subject.add_observer(observers[i].get());
            subject_with_param.add_observer(observers[i].get());
            subject_with_complex_param.add_observer(observers[i].get());
        }
    });

    b.run("fastsignal", [&]() {
        for (int i = 0; i < OBSERVERS_COUNT; ++i) {
            subject.sig.add<&ObserverI::set_global_value1>(observers[i].get());
            subject_with_param.sig.add<&ObserverI::set_global_value2>(observers[i].get());
            subject_with_complex_param.sig.add<&ObserverI::set_global_value3>(observers[i].get());
        }
    });

    b.run("fteng_sig", [&]() {
        for (int i = 0; i < OBSERVERS_COUNT; ++i) {
            subject.fteng_sig.connect<&ObserverI::set_global_value1>(observers[i].get());
            subject_with_param.fteng_sig.connect<&ObserverI::set_global_value2>(observers[i].get());
            subject_with_complex_param.fteng_sig.connect<&ObserverI::set_global_value3>(observers[i].get());
        }
    });
}

void benchmark_notify_observers_without_param(SubjectWithoutParam& subject)
{
    ankerl::nanobench::Bench b;
    b.title("benchmark_notify_observers_without_param").relative(true).performanceCounters(true);
    b.minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers();
    });

    b.run("fastsignal", [&]() {
        subject.notify_sig();
    });

    b.run("fteng_sig", [&]() {
        subject.notify_sig_m();
    });
}

void benchmark_notify_observers_param(SubjectWithParam& subject)
{
    ankerl::nanobench::Bench b;
    b.title("benchmark_notify_observers_param").relative(true).performanceCounters(true);
    b.minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers(1);
    });

    b.run("fastsignal", [&]() {
        subject.notify_sig(1);
    });

    b.run("fteng_sig", [&]() {
        subject.notify_sig_m(1);
    });
}

void benchmark_notify_observers_complex_param(SubjectWithComplexParam& subject)
{
    ankerl::nanobench::Bench b;
    b.title("benchmark_notify_observers_complex_param").relative(true).performanceCounters(true);
    b.minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers(complex_param);
    });

    b.run("fastsignal", [&]() {
        subject.notify_sig(complex_param);
    });

    b.run("fteng_sig", [&]() {
        subject.notify_sig_m(complex_param);
    });
}

int main()
{
    SubjectWithoutParam subject;
    SubjectWithParam subject_with_param;
    SubjectWithComplexParam subject_with_complex_param;

    create_observers();

    benchmark_add_observer(subject, subject_with_param, subject_with_complex_param);
    benchmark_notify_observers_without_param(subject);
    benchmark_notify_observers_param(subject_with_param);
    benchmark_notify_observers_complex_param(subject_with_complex_param);

    return 0;
}
