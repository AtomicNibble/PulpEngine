#include "stdafx.h"

#include <Threading\FixedThreadQue.h>
#include <Threading\Thread.h>

TEST(Threading, FixedThreadQue)
{
    typedef core::FixedThreadQue<int, 4, core::CriticalSection> QueueType;
    QueueType queue;

    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.push(4);

    core::Thread thread;
    thread.Create("FixedQueueTest");
    thread.setData(&queue);
    thread.Start([](const core::Thread& thread) -> core::Thread::ReturnValue {
        QueueType* pQueue = reinterpret_cast<QueueType*>(thread.getData());

        pQueue->push(5);
        pQueue->push(6);
        pQueue->push(7);
        pQueue->push(8);

        return 0;
    });

    // give some time for thread to call first push where it gets blocked.
    core::Thread::Sleep(20);

    // pop all the values.
    int32_t val;
    EXPECT_TRUE(queue.tryPop(val));
    EXPECT_EQ(1, val);

    core::Thread::Sleep(0);

    EXPECT_TRUE(queue.tryPop(val));
    EXPECT_EQ(2, val);
    EXPECT_TRUE(queue.tryPop(val));
    EXPECT_EQ(3, val);
    EXPECT_TRUE(queue.tryPop(val));
    EXPECT_EQ(4, val);

    // thread should be able to join.
    thread.Join();

    EXPECT_EQ(5, queue.pop());
    EXPECT_EQ(6, queue.pop());

    queue.pop(val);
    EXPECT_EQ(7, val);

    queue.pop(val);
    EXPECT_EQ(8, val);
}