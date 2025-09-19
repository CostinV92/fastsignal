#include <benchmark/benchmark.h>

#include "bench_base.hpp"
#include "fastsignal.hpp"
#include <signals.hpp>

using namespace fastsignal;

void setup([[maybe_unused]] const benchmark::State& state)
{
    static bool created = false;

    if (created)
        return;

    create_observers();
    created = true;
}

static void BM_observer_call(benchmark::State& state)
{
    for (auto _ : state) {
        for (auto &observer : observers)
            observer->handler1_v();
    }
}
BENCHMARK(BM_observer_call)->Setup(setup)->Name("observers_call");

static void BM_sig_call(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig();
    }
}
BENCHMARK(BM_sig_call)->Setup(setup)->Name("sig_call");

static void BM_fteng_sig_call(benchmark::State& state)
{

    for (auto _ : state) {
        subject.fteng_sig();
    }
}
BENCHMARK(BM_fteng_sig_call)->Setup(setup)->Name("fteng_sig_call");

static void BM_notify_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers();
    }
}
BENCHMARK(BM_notify_observers)->Setup(setup)->Name("notify_observers()");

static void BM_sig_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_observers();
    }
}
BENCHMARK(BM_sig_observers)->Setup(setup)->Name("sig_observers()");

static void BM_fteng_sig_observers(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers();
    }
}
BENCHMARK(BM_fteng_sig_observers)->Setup(setup)->Name("fteng_sig_observers()");

static void BM_notify_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers(0.005f);
    }
}
BENCHMARK(BM_notify_observers2)->Setup(setup)->Name("notify_observers(double)");

static void BM_sig_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_observers(0.005f);
    }
}
BENCHMARK(BM_sig_observers2)->Setup(setup)->Name("sig_observers(double)");

static void BM_fteng_sig_observers2(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers(0.005f);
    }
}
BENCHMARK(BM_fteng_sig_observers2)->Setup(setup)->Name("fteng_sig_observers(double)");

static void BM_notify_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.notify_observers(complex_param);
    }
}
BENCHMARK(BM_notify_observers3)->Setup(setup)->Name("notify_observers(complex_param)");

static void BM_sig_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.sig_observers(complex_param);
    }
}
BENCHMARK(BM_sig_observers3)->Setup(setup)->Name("sig_observers(complex_param)");

static void BM_fteng_sig_observers3(benchmark::State& state)
{
    for (auto _ : state) {
        subject.fteng_sig_observers(complex_param);
    }
}
BENCHMARK(BM_fteng_sig_observers3)->Setup(setup)->Name("fteng_sig_observers(complex_param)");
