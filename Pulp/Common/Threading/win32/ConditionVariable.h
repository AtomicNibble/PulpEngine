#pragma once

#ifndef _X_THREADING_SYNC_CONDIRIONAL_H_
#define _X_THREADING_SYNC_CONDIRIONAL_H_

X_NAMESPACE_BEGIN(core)

class CriticalSection;
class SharedLock;

class ConditionVariable
{
public:
    ConditionVariable(void);

    // notify one thread that is sleeping on the condition to wake up.
    void NotifyOne(void);
    // notify all threads that are sleeping on the condition to wake up.
    void NotifyAll(void);

    // releease the critical section lock, and puts thread to sleep until it is notified.
    void Wait(CriticalSection& criticalSection);

    // releease the shared lock, and puts thread to sleep until it is notified.
    // it's up to you to know if a exlusive or shared lock was taken in the contex of this call.
    // if the lock has been taken with 'EnterShared' you must set exclusive to false.
    void Wait(SharedLock& sharedLock, bool isExclusive);

private:
    CONDITION_VARIABLE condVar_;
};

X_NAMESPACE_END

#endif // !_X_THREADING_SYNC_CONDIRIONAL_H_
