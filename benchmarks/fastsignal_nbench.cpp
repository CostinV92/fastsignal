#include <nanobench.h>

#include "bench_base.hpp"
#include "fastsignal.hpp"
#include <signals.hpp>

using namespace fastsignal;

void bench_call(Subject& subject)
{
    ankerl::nanobench::Bench b;
    b.title("bench_call").relative(true).minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        for (auto& observer : observers)
            observer->handler1_v();
    });

    b.run("fastsignal", [&]() {
        subject.sig();
    });

    b.run("fteng_sig", [&]() {
        subject.fteng_sig();
    });
}

void bench_no_param(Subject& subject)
{
    ankerl::nanobench::Bench b;
    b.title("bench_no_param").relative(true).minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers();
    });

    b.run("fastsignal", [&]() {
        subject.sig_observers();
    });

    b.run("fteng_sig", [&]() {
        subject.fteng_sig_observers();
    });
}

void bench_param(Subject& subject)
{
    ankerl::nanobench::Bench b;
    b.title("bench_param").relative(true).minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers(0.005);
    });

    b.run("fastsignal", [&]() {
        subject.sig_observers(0.005);
    });

    b.run("fteng_sig", [&]() {
        subject.fteng_sig_observers(0.005);
    });
}

void bench_complex_param(Subject& subject)
{
    ankerl::nanobench::Bench b;
    b.title("bench_complex_param").relative(true).minEpochIterations(ITERATIONS);
    b.run("observer", [&]() {
        subject.notify_observers(complex_param);
    });

    b.run("fastsignal", [&]() {
        subject.sig_observers(complex_param);
    });

    b.run("fteng_sig", [&]() {
        subject.fteng_sig_observers(complex_param);
    });
}

int main()
{
    create_observers();

    bench_call(subject);

    bench_no_param(subject);
    bench_param(subject);
    bench_complex_param(subject);

    return 0;
}
