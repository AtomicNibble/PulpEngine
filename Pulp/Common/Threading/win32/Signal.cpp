#include <EngineCommon.h>
#include "Signal.h"



X_NAMESPACE_BEGIN(core)

Signal::Signal(bool autoReset) :
	hHandle_(0)
{
	BOOL manualReset = !autoReset;

	hHandle_ = CreateEvent(NULL, manualReset, FALSE, NULL);
	if (hHandle_ == NULL) {
		core::lastError::Description Dsc;
		X_ERROR("Signal", "failed to create event. Error: %s", core::lastError::ToString(Dsc));
	}
}

Signal::~Signal()
{
	CloseHandle(hHandle_);
}


void Signal::raise(void)
{
	SetEvent(hHandle_);
}

void Signal::clear(void)
{
	ResetEvent(hHandle_);
}

bool Signal::wait(uint32_t timeout)
{
	DWORD result = WaitForSingleObject(hHandle_, timeout == Signal::WAIT_INFINITE ? INFINITE : timeout);

	if (result != WAIT_OBJECT_0)
	{
		core::lastError::Description Dsc;
		if (timeout == Signal::WAIT_INFINITE) {
			X_ERROR("Signal", "WaitForObject infinite error: %s", core::lastError::ToString(Dsc));
		}
		else if (result != WAIT_TIMEOUT) {
			X_ERROR("Signal", "WaitForObject none-infinite error: %s", core::lastError::ToString(Dsc));
		}
	}

	return result == WAIT_OBJECT_0;
}



X_NAMESPACE_END
