#pragma once

#ifndef _X_THREAD_H_
#define _X_THREAD_H_

#include <Traits/FunctionTraits.h>
#include <Util\Delegate.h>
#include <Util\LastError.h>

#include <String\StackString.h>

#ifdef YieldProcessor
#undef YieldProcessor
#endif // !YieldProcessor

X_NAMESPACE_BEGIN(core)


X_DISABLE_WARNING(4324)

X_ALIGNED_SYMBOL(class Thread, 64)
{
public:
    struct State
    {
        enum Enum
        {
            NOT_CREATED = 0, // The OS thread has not been created yet.
            READY = 1,       // The thread is ready to run.
            RUNNING = 2,     // The thread is running.
            STOPPING = 3,    // The thread should stop running.
            FINISHED = 4     // The thread has finished running.
        };
    };

    X_DECLARE_FLAGS(CpuCore)
    (
        CORE0, CORE1, CORE2, CORE3,
        CORE4, CORE5, CORE6, CORE7,
        CORE8, CORE9, CORE10, CORE11,
        CORE12, CORE13, CORE14, CORE15);

    X_DECLARE_ENUM(FPE)
    (
        NONE,  // nope . nope
        BASIC, // Invalid operation, Div by Zero!
        ALL    // Invalid operation, Div by Zero, Denormalized operand, Overflow, UnuderFlow, Unexact
    );

    X_DECLARE_ENUM(Priority)
    (
        LOWEST,
        BELOW_NORMAL,
        NORMAL,
        ABOVE_NORMAL,
        HIGHEST,
        REALTIME,
        IDLE);

    static const uint32_t INFINITE_SLEEP = INFINITE;

public:
    typedef Flags<CpuCore> AffinityFlags;
    typedef uint32_t ReturnValue;
    typedef traits::Function<ReturnValue(const Thread&)> Function;

    Thread(void);  // no thread is created.
    ~Thread(void); /// Calls Destroy() to stop and join the thread.

    void Create(const char* pName, uint32_t stackSize = 0);

    /// \brief Destroys the OS thread by calling Stop() and Join().
    /// \remark A new OS thread must be created before the thread can be started again.
    void Destroy(void);

    void Start(Function::Pointer function); // runs the thread with given function
    void Stop(void);                        // tells the thread to stop dose not wait.
    void Join(void);                        // waits till thread has finished.

    bool ShouldRun(void) const volatile;
    bool HasFinished(void) const volatile;

    bool SetThreadAffinity(const AffinityFlags flags);
    void SetFPE(FPE::Enum fpe);

    X_INLINE uint32_t GetID(void) const;
    X_INLINE State::Enum GetState(void) const;

    void CancelSynchronousIo(void);

    X_INLINE void setData(void* pData);
    X_INLINE void* getData(void) const;

    X_INLINE static void Sleep(uint32_t milliSeconds);
    static bool SleepAlertable(uint32_t milliSeconds = INFINITE_SLEEP); // returns true if alerted
    X_INLINE static void Yield(void);
    X_INLINE static void YieldProcessor(void);
    X_INLINE static void BackOff(int32_t backoffCount);
    X_INLINE static uint32_t GetCurrentID(void);
    static void Join(uint32_t threadId);
    static void SetName(uint32_t threadId, const char* name);

    static Priority::Enum GetPriority(void);
    static bool SetPriority(Priority::Enum priority);
    static void SetFPE(uint32_t threadId, FPE::Enum fpe);

protected:
    HANDLE createThreadInternal(uint32_t stackSize, LPTHREAD_START_ROUTINE func);

private:
    static uint32_t __stdcall ThreadFunction_(void* threadInstance);

protected:
    HANDLE handle_;
    uint32_t id_;
    Function::Pointer function_;
    State::Enum state_;
    void* pData_;

    core::StackString<64> name_;
};

template<class T>
class ThreadMember : private Thread
{
public:
    typedef core::Delegate<ReturnValue(const Thread&)> FunctionDelagate;

public:
    using Thread::Thread;

    X_INLINE void Create(const char* pName, uint32_t stackSize = 0);
    X_INLINE void Start(FunctionDelagate delagate);

    X_INLINE void Stop(void); // tells the thread to stop dose not wait.
    X_INLINE void Join(void); // waits till thread has finished.

    X_INLINE bool ShouldRun(void) const volatile;
    X_INLINE bool HasFinished(void) const volatile;

    X_INLINE bool SetThreadAffinity(const AffinityFlags flags);
    X_INLINE void SetFPE(FPE::Enum fpe);

    X_INLINE uint32_t GetID(void) const;
    X_INLINE State::Enum GetState(void) const;

    X_INLINE void setData(void* pData);
    X_INLINE void* getData(void) const;

private:
    static uint32_t __stdcall ThreadFunctionDel_(void* threadInstance);

private:
    FunctionDelagate delagate_;
};

X_ENABLE_WARNING(4324)

// member thread
class ThreadAbstract
{
public:
    ThreadAbstract();

    void Create(const char* name, uint32_t stackSize = 0);
    void Start(void); // runs the thread
    void Stop(void);  // tells the thread to stop dose not wait.
    void Join(void);  // waits till thread has finished.

    uint32_t GetID(void) const;
    Thread::State::Enum GetState(void) const;

    void CancelSynchronousIo(void);

protected:
    virtual ~ThreadAbstract();
    virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_ABSTRACT;

private:
    static Thread::ReturnValue ThreadFunc(const Thread& thread);

    Thread thread_;
};

X_NAMESPACE_END

#include "Thread.inl"

#endif // !_X_THREAD_H_
