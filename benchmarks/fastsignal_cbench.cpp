#include <sbench.hpp>

#include "bench_base.hpp"
#include "fastsignal.hpp"
#include <signals.hpp>

void bench_call(Subject& subject)
{
    sbench::SBench bench("bench_call");
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
    sbench::SBench bench("bench_no_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers();
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers();
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig_observers();
    });
}

void bench_param(Subject& subject)
{
    sbench::SBench bench("bench_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers(0.005);
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers(0.005);
    });

    bench.run("fteng_sig", [&]() {
        subject.fteng_sig_observers(0.005);
    });
}

void bench_complex_param(Subject& subject)
{
    sbench::SBench bench("bench_complex_param");
    bench.iteration(ITERATIONS);

    bench.run("observer", [&]() {
        subject.notify_observers(complex_param);
    });

    bench.run("fastsignal", [&]() {
        subject.sig_observers(complex_param);
    });

    bench.run("fteng_sig", [&]() {
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