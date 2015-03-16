#include <EngineCommon.h>
#include "Thread.h"



X_NAMESPACE_BEGIN(core)


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

	if (handle_ == NULL)
	{
		lastError::Description Dsc;
		X_ERROR("Thread", "failed to create thread. Erorr: %s", lastError::ToString(Dsc));
	}
	else
		state_ = State::READY;
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
}

void Thread::Stop(void)
{
	if (state_ != State::READY)
		state_ = State::STOPPING;
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

uint32_t __stdcall Thread::ThreadFunction_(void* threadInstance)
{
	Thread* pThis = (Thread*)threadInstance;

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


void ThreadAbstract::Start(void)
{
	thread_.Create("AbstractThread");
	thread_.setData(this);
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


Thread::ReturnValue ThreadAbstract::ThreadFunc(const Thread& thread)
{
	ThreadAbstract* pThis = (ThreadAbstract*)thread.getData();
	return pThis->ThreadRun(thread);
}



X_NAMESPACE_END