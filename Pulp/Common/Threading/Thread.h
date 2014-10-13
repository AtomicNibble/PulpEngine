#pragma once


#ifndef _X_THREAD_H_
#define _X_THREAD_H_

#include <Traits/FunctionTraits.h>

X_NAMESPACE_BEGIN(core)

/// \code
///   core::Thread::ReturnValue ThreadFunction(const core::Thread& thread)
///   {
///     while (thread.ShouldRun())
///     {
///       // do something over and over again
///       // ...
///
///       // either grant resources to other CPUs explicitly
///       core::Thread::Yield();
///       or
///       core::Thread::Sleep();
///
///       // or grant resources implicitly by waiting for e.g. condition variables
///   	}
///   
///   	return core::Thread::ReturnValue(0);
///   }
///
///   // the thread is created and started somewhere else, probably from the main thread
///   core::Thread thread;
///   thread.Create(65536, "Test thread");
///   thread.Start(&ThreadFunction);
///   // ThreadFunction is now run in a different thread, continuing to work until Stop() is called
///
///   // do other work
///   ...
///
///   // some time later
///   // calling Stop() results in making ShouldRun() returning false
///   thread.Stop();
///
///   // wait until the thread has finished
///   thread.Join();
/// \endcode
class Thread
{
	struct State
	{
		enum Enum
		{
			NOT_CREATED = 0,		// The OS thread has not been created yet.
			READY = 1,				// The thread is ready to run.
			RUNNING = 2,			// The thread is running.
			STOPPING = 3,			// The thread should stop running.
			FINISHED = 4			// The thread has finished running.
		};
	};

public:
	typedef uint32_t ReturnValue;
	typedef traits::Function<ReturnValue(const Thread&)> Function;

	Thread(void); // no thread is created.
	~Thread(void); /// Calls Destroy() to stop and join the thread.

	void Create(const char* name, uint32_t stackSize = 0);

	/// \brief Destroys the OS thread by calling Stop() and Join().
	/// \remark A new OS thread must be created before the thread can be started again.
	void Destroy(void);

	void Start(Function::Pointer function); // runs the thread with given function
	void Stop(void); // tells the thread to stop dose not wait.
	void Join(void); // waits till thread has finished.

	bool ShouldRun(void) const volatile;
	bool HasFinished(void) const volatile;

	void setData(void* pData) {
		pData_ = pData;
	}

	void* getData() const {
		return pData_;
	}

	static void Sleep(uint32_t milliSeconds);
	static void Yield(void);

private:
	static unsigned int __stdcall ThreadFunction_(void* threadInstance);

	HANDLE handle_;
	unsigned int id_;
	Function::Pointer function_;
	State::Enum state_;
	void* pData_;
};



// member thread
class ThreadAbstract
{
public:
	ThreadAbstract();

	void Start(void);	 // runs the thread
	void Stop(void); // tells the thread to stop dose not wait.
	void Join(void); // waits till thread has finished.

protected:
	virtual ~ThreadAbstract();
	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_ABSTRACT;

private:
	static Thread::ReturnValue ThreadAbstract::ThreadFunc(const Thread& thread);

	Thread thread_;
};

X_NAMESPACE_END


#endif // !_X_THREAD_H_
