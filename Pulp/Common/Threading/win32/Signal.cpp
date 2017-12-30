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

bool Signal::wait(uint32_t timeoutMS, bool alertable)
{
	DWORD result = WaitForSingleObjectEx(hHandle_, timeoutMS == Signal::WAIT_INFINITE ? INFINITE : timeoutMS, alertable);

	if (result != WAIT_OBJECT_0)
	{
		if (alertable && result == WAIT_IO_COMPLETION) {
			return true;
		}

		core::lastError::Description Dsc;
		if (timeoutMS == Signal::WAIT_INFINITE) {
			X_ERROR("Signal", "WaitForObject infinite error: %s", core::lastError::ToString(Dsc));
		}
		else if (result != WAIT_TIMEOUT) {
			X_ERROR("Signal", "WaitForObject none-infinite error: %s", core::lastError::ToString(Dsc));
		}
	}

	return result == WAIT_OBJECT_0;
}



X_NAMESPACE_END
