#include "stdafx.h"

#include <Random\XorShift.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    static const int THREAD_NUM = 6;
    static const int LOOP_COUNT = 5;

    static ThreadLocalStorage g_local;
    static Semaphore g_Sema(0, THREAD_NUM);
    static AtomicInt g_int(0);
    static AtomicInt g_entryCondition(0);
    static Spinlock g_spinlock;
    static CriticalSection g_critical;
    static ConditionVariable g_cond;

    static int g_protectedInt = 0;
    static int g_nonProtectedInt = 0;

    X_ALIGNED_SYMBOL(static int32_t g_atomicInt, 4) = 0;
    static int32_t g_nonAtomicInt = 0;

    // --------------------------------------------

    static Thread::ReturnValue ThreadAtomic_add(const Thread& thread)
    {
        for (unsigned int i = 0; i < LOOP_COUNT; ++i) {
            // makes it very likley valeus will override.
            {
                int local = g_nonAtomicInt;
                local += 64;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_nonAtomicInt = local;
            }

            Thread::Sleep(gEnv->xorShift.rand() % 5);
            atomic::Add(&g_atomicInt, 64);
        }
        return Thread::ReturnValue(0);
    }
    static Thread::ReturnValue ThreadAtomic_decrement(const Thread& thread)
    {
        for (unsigned int i = 0; i < LOOP_COUNT; ++i) {
            // makes it very likley valeus will override.
            {
                int local = g_nonAtomicInt;
                --local;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_nonAtomicInt = local;
            }

            Thread::Sleep(gEnv->xorShift.rand() % 5);
            atomic::Decrement(&g_atomicInt);
        }
        return Thread::ReturnValue(0);
    }
    static Thread::ReturnValue ThreadAtomic_increment(const Thread& thread)
    {
        for (unsigned int i = 0; i < LOOP_COUNT; ++i) {
            // makes it very likley valeus will override.
            {
                int local = g_nonAtomicInt;
                ++local;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_nonAtomicInt = local;
            }

            Thread::Sleep(gEnv->xorShift.rand() % 5);
            atomic::Increment(&g_atomicInt);
        }
        return Thread::ReturnValue(0);
    }

    // --------------------------------------------

    static Thread::ReturnValue ThreadFunction(const Thread& thread)
    {
        while (thread.ShouldRun()) {
            g_nonProtectedInt++;
            Thread::Sleep(10);
        }

        return Thread::ReturnValue(0);
    }

    static Thread::ReturnValue ThreadFunction_tls(const Thread& thread)
    {
        g_local.SetValue(nullptr);

        return Thread::ReturnValue(0);
    }

    static Thread::ReturnValue ThreadFunction_semaphone(const Thread&)
    {
        g_Sema.AcquireSlot();
        ++g_int;
        return Thread::ReturnValue(0);
    }

    static Thread::ReturnValue ThreadFunction_spinlock(const Thread&)
    {
        for (unsigned int i = 0; i < LOOP_COUNT; ++i) {
            // make sure that the operation is not atomic
            int local = g_nonProtectedInt;
            ++local;
            Thread::Sleep(gEnv->xorShift.rand() % 5);
            g_nonProtectedInt = local;

            {
                Spinlock::ScopedLock lock(g_spinlock);

                int local = g_protectedInt;
                ++local;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_protectedInt = local;
            }
        }

        return Thread::ReturnValue(0);
    }

    static Thread::ReturnValue ThreadFunction_criticalsection(const Thread&)
    {
        for (unsigned int i = 0; i < LOOP_COUNT; ++i) {
            // make sure that the operation is not atomic
            int local = g_nonProtectedInt;
            ++local;
            Thread::Sleep(gEnv->xorShift.rand() % 5);
            g_nonProtectedInt = local;

            {
                CriticalSection::ScopedLock lock(g_critical);

                int local = g_protectedInt;
                ++local;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_protectedInt = local;
            }
        }

        return Thread::ReturnValue(0);
    }

    static Thread::ReturnValue ThreadFunction_conditional(const Thread&)
    {
        for (int i = 0; i < LOOP_COUNT; ++i) {
            {
                CriticalSection::ScopedLock lock(g_critical);

                // wait until the condition variable is signaled
                while (g_entryCondition == 0) {
                    g_cond.Wait(g_critical);
                }

                // the critical section is now owned by us
                int local = g_protectedInt;
                ++local;
                Thread::Sleep(gEnv->xorShift.rand() % 5);
                g_protectedInt = local;

                g_entryCondition = 0;
            }
        }

        return Thread::ReturnValue(0);
    }

    static void RunThreads(Thread::Function::Pointer func)
    {
        Thread threads[THREAD_NUM];

        for (int i = 0; i < THREAD_NUM; i++)
            threads[i].Create("Test Thread");
        for (int i = 0; i < THREAD_NUM; i++)
            threads[i].Start(func);

        for (int i = 0; i < THREAD_NUM; i++)
            threads[i].Stop();
        for (int i = 0; i < THREAD_NUM; i++)
            threads[i].Join();
    }

} // namespace

TEST(Threading, Atomic)
{
    // we want to do a non attomic goat.

    int expected = THREAD_NUM * LOOP_COUNT;

    g_atomicInt = 0;
    RunThreads(ThreadAtomic_add);
    EXPECT_TRUE(g_atomicInt == (expected * 64));

    g_atomicInt = 0;
    RunThreads(ThreadAtomic_decrement);
    EXPECT_TRUE(-g_atomicInt == expected);

    g_atomicInt = 0;
    RunThreads(ThreadAtomic_increment);
    EXPECT_TRUE(g_atomicInt == expected);
}

TEST(Threading, Threads)
{
    Thread threads[THREAD_NUM];

    g_nonProtectedInt = 0;

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Create("Test Thread");
    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Start(ThreadFunction);

    Thread::Sleep(50);

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Stop();
    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Join();

    int val = g_nonProtectedInt;
    EXPECT_TRUE(val > 5);
}

TEST(Threading, LocalStorage)
{
    int dead = 0xdead;

    g_local.SetValue(&dead);

    RunThreads(ThreadFunction_tls);

    EXPECT_TRUE(g_local.GetValue<int>() == &dead);
}

TEST(Threading, Semaphore)
{
    Thread threads[THREAD_NUM];

    g_int = 0;

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Create("Test Thread");

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Start(ThreadFunction_semaphone);

    for (int i = 0; i < THREAD_NUM; i++)
        g_Sema.ReleaseSlot();

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Join();

    EXPECT_TRUE(g_int == THREAD_NUM);
}

TEST(Threading, SpinLock)
{
    g_protectedInt = 0;
    g_nonProtectedInt = 0;

    RunThreads(ThreadFunction_spinlock);

    int val = g_protectedInt / LOOP_COUNT;

    EXPECT_TRUE(val == THREAD_NUM);
}

TEST(Threading, CriticalSection)
{
    g_protectedInt = 0;
    g_nonProtectedInt = 0;

    RunThreads(ThreadFunction_criticalsection);

    int val = g_protectedInt / LOOP_COUNT;

    EXPECT_TRUE(val == THREAD_NUM);
}

TEST(Threading, Conditional)
{
    Thread threads[THREAD_NUM];

    g_protectedInt = 0;

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Create("Test Thread");

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Start(ThreadFunction_conditional);

    uint32_t finishedCount = 0;
    while (finishedCount != THREAD_NUM) {
        finishedCount = 0;
        for (uint32_t i = 0; i < THREAD_NUM; ++i) {
            if (threads[i].HasFinished())
                ++finishedCount;
        }

        // signal one thread to enter the the critical section
        g_entryCondition = 1;
        g_cond.NotifyOne();
        Thread::Yield();
    }

    for (int i = 0; i < THREAD_NUM; i++)
        threads[i].Join();

    int val = g_protectedInt / LOOP_COUNT;

    EXPECT_TRUE(val == THREAD_NUM);
}
