#include "stdafx.h"

#include <Math\XHalf.h>

void BM_quat_set_mat33(benchmark::State& state)
{
    Matrix33f m0(
        -0.2353829f, 0.3812674f, 0.8939967f,
        0.0385857f, -0.9154506f, 0.4005763f,
        0.9711365f, 0.1287843f, 0.2007700f, true
    );

    Matrix33f m1(
        -0.4480736f, -0.0000000f * state.thread_index, 0.8939967f,
        0.7992300f, -0.4480736f, 0.4005763f,
        0.4005763f, 0.8939967f, 0.2007700f, true
    );

    Matrix33f m2(
        -0.2353829f, 0.3812674f, 0.8939967f,
        0.0385857f, -0.9154506f, 0.4005763f,
        0.9711365f, 0.1287843f, 0.2007700f, true
    );

    Quatf q0;
    Quatf q1;
    Quatf q2;

    while (state.KeepRunning()) {
        q0.set(m0);
        q1.set(m1);
        q2.set(m2);

        benchmark::DoNotOptimize(q0 * q1 * q2);
    }
}

BENCHMARK(BM_quat_set_mat33);