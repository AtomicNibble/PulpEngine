#include <EngineCommon.h>
#include "Thread.h"

#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)
#pragma intrinsic(__yield)
#else
#pragma intrinsic(_mm_pause)
#endif

X_NAMESPACE_BEGIN(core)

namespace
{

	const DWORD MS_VC_EXCEPTION = 0x406D1388;

	X_PACK_PUSH(8)
	struct THREADNAME_INFO
	{
		DWORD dwType;		// Must be 0x1000.
		LPCSTR szName;		// Pointer to name (in user addr space).
		DWORD dwThreadID;	// Thread ID (-1=caller thread).
		DWORD dwFlags;		// Reserved for future use, must be zero.
	};
	X_PACK_POP;

	void SetThreadName(DWORD dwThreadID, const char* threadName)
	{
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

		__try
		{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{

		}
	}
}


Thread::Thread(void) :
handle_(NULL),
id_(0),
function_(nullptr),
state_(State::NOT_CREATED)
{

}

Thread::~Thread(void)
{

}

void Thread::Create(const char* name, uint32_t stackSize )
{
	handle_ = CreateThread(NULL, stackSize, (LPTHREAD_START_ROUTINE)ThreadFunction_, 
					this, CREATE_SUSPENDED, (LPDWORD)&id_);

	name_.set(name);

	if (handle_ == NULL)
	{
		lastError::Description Dsc;
		X_ERROR("Thread", "failed to create thread. Erorr: %s", lastError::ToString(Dsc));
	}
	else {
		state_ = State::READY;
	}
}

void Thread::Destroy(void)
{
	Stop();
	Join();
}

void Thread::Start(Function::Pointer function)
{
	function_ = function;
	state_ = State::RUNNING;

	if (ResumeThread(handle_) == (DWORD)-1)
	{
		lastError::Description Dsc;
		X_ERROR("Thread", "failed to start thread. Erorr: %s", lastError::ToString(Dsc));
	}

	SetThreadName(GetID(), name_.c_str());
}

void Thread::Stop(void)
{
	if (state_ != State::READY) {
		state_ = State::STOPPING;
	}
}

void Thread::Join(void)
{
	if (state_ != State::READY && handle_ != NULL)
	{
		if (WaitForSingleObject(handle_, INFINITE) == WAIT_FAILED)
		{
			lastError::Description Dsc;
			X_ERROR("Thread", "thread join failed. Erorr: %s", lastError::ToString(Dsc));
		}
	}
	handle_ = NULL;
	state_ = State::FINISHED;
	id_ = 0;
}

bool Thread::ShouldRun(void) const volatile
{
	return state_ == State::RUNNING;
}

bool Thread::HasFinished(void) const volatile
{
	return state_ == State::FINISHED;
}


bool Thread::SetThreadAffinity(const AffinityFlags flags)
{
	DWORD_PTR mask = flags.ToInt();

	if (SetThreadAffinityMask(handle_, mask) == 0)
	{
		lastError::Description Dsc;
		X_ERROR("Failed to set thread affinity. Error: %s", lastError::ToString(Dsc));
		return false;
	}

	return true;
}

uint32_t Thread::GetID(void) const
{
	return ::GetThreadId(handle_);
}

// static
void Thread::Sleep(uint32_t milliSeconds)
{
	GoatSleep(milliSeconds);
}

void Thread::Yield(void)
{
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms686352(v=vs.85).aspx
	SwitchToThread();
}

void Thread::YieldProcessor(void)
{
	// about a 9 cycle delay
#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)
	__yield();
#else
	_mm_pause();
#endif
}


uint32_t Thread::GetCurrentID(void)
{
	return ::GetCurrentThreadId();
}

void Thread::SetName(uint32_t threadId, const char* name)
{
	SetThreadName(threadId, name);
}


uint32_t __stdcall Thread::ThreadFunction_(void* threadInstance)
{
	Thread* pThis = reinterpret_cast<Thread*>(threadInstance);

	uint32_t ret = pThis->function_(*pThis);
	pThis->state_ = State::FINISHED;
	return ret;
}

// ~static

// -------------------------------------------------------------------------------


ThreadAbstract::ThreadAbstract()
{

}

ThreadAbstract::~ThreadAbstract()
{
	thread_.Destroy();
}

void ThreadAbstract::Create(const char* name, uint32_t stackSize)
{
	thread_.Create(name ? name : "AbstractThread", stackSize);
	thread_.setData(this);
}

void ThreadAbstract::Start(void)
{
	thread_.Start(ThreadFunc);
}

void ThreadAbstract::Stop(void)
{
	thread_.Stop();
}

void ThreadAbstract::Join(void)
{
	thread_.Join();
}

uint32_t ThreadAbstract::GetID(void) const
{
	return thread_.GetID();
}

Thread::ReturnValue ThreadAbstract::ThreadFunc(const Thread& thread)
{
	ThreadAbstract* pThis = reinterpret_cast<ThreadAbstract*>(thread.getData());
	return pThis->ThreadRun(thread);
}



X_NAMESPACE_END