#pragma once

#ifndef _X_THREADING_SEMAPHORE_H_
#define _X_THREADING_SEMAPHORE_H_

X_NAMESPACE_BEGIN(core)

///
///	Used to restrict access to N threads.
///
///
//
//	const size_t THREAD_NUM = 5;
//	const size_t SEM_MAX = 3;
//
//
//	core::Semaphore seamaphore(SEM_MAX, SEM_MAX);
//
//	void test(void)
//	{
//		core::Thread threads[THREAD_NUM];
//
//		for (int i = 0; i < THREAD_NUM; i++)
//			threads[i].Create("Test Thread");
//
//		for (int i = 0; i < THREAD_NUM; i++)
//			threads[i].Start(ThreadFunction);
//
//		for (int i = 0; i < THREAD_NUM; i++)
//			threads[i].Join();
//	}
//
//
//	core::Thread::ReturnValue ThreadFunction(const core::Thread& thread)
//	{
//		while (thread.ShouldRun())
//		{
//			// wait for a slot for the calling thread.
//			seamaphore.AcquireSlot();
//
//			// Do Work..
//
//			// release slot, for another thread to pick up.
//			seamaphore.ReleaseSlot();
//		}
//
//		return core::Thread::ReturnValue(0);
//	}
//

class Semaphore
{
public:
    Semaphore(int initialValue, int maximumValue);
    X_INLINE ~Semaphore(void);

    // release a slot, allowing other threads that call wait to take a slot.
    X_INLINE void ReleaseSlot(void);

    // wait untill we have a free slot.
    X_INLINE void AcquireSlot(void);

    // see if we can
    X_INLINE bool TryAcquireSlot(void);

private:
    HANDLE sema_;
};

#include "Semaphore.inl"

X_NAMESPACE_END

#endif // !_X_THREADING_SEMAPHORE_H_
