#include "stdafx.h"
#include "OsFileAsyncOperation.h"

#include <Util\LastError.h>


X_NAMESPACE_BEGIN(core)

XOsFileAsyncOperation::XOsFileAsyncOperation(MemoryArenaBase* arena, HANDLE hFile, size_t position) :
hFile_(hFile),
overlapped_(X_NEW(ReferenceCountedOverlapped, arena, "OVERLAPPED"), arena)
{
	LPOVERLAPPED pOverlapped = overlapped_.instance();
	core::zero_this(pOverlapped);

	LARGE_INTEGER large;
	large.QuadPart = position;

	pOverlapped->Offset = large.LowPart;
	pOverlapped->OffsetHigh = large.HighPart;
}


bool XOsFileAsyncOperation::hasFinished(uint32_t* pNumBytes) const
{
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_.instance(), &bytesTransferred, false)) {
		if (pNumBytes) {
			*pNumBytes = static_cast<uint32_t>(bytesTransferred);
		}
		return true;
	}
	else if (core::lastError::Get() == ERROR_IO_INCOMPLETE)
	{
		return false;
	}
	else
	{
		// some goaty error
		core::lastError::Description Dsc;
		X_ERROR("AsyncFile", "Failed to check if async request has finsihed. Error: %s", core::lastError::ToString(Dsc));
	}
	
	return false;
}


uint32_t XOsFileAsyncOperation::waitUntilFinished(void) const
{
	// same as above but with bWait = true;
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_.instance(), &bytesTransferred, true))
	{
		return safe_static_cast<uint32_t, DWORD>(bytesTransferred);
	}
	else
	{
		// some goaty error
		core::lastError::Description Dsc;
		X_ERROR("AsyncFile", "Failed to wait until async request has finsihed. Error: %s", core::lastError::ToString(Dsc));
	}

	// nope.
	return 0;
}

void XOsFileAsyncOperation::cancel(void)
{
	DWORD bytesTransferred = 0;

	if (::CancelIoEx(hFile_, overlapped_.instance()))
	{
		// wait for it to finish.
		if (!::GetOverlappedResult(hFile_, overlapped_.instance(), &bytesTransferred, true))
		{
			if (core::lastError::Get() != ERROR_OPERATION_ABORTED)
			{
				core::lastError::Description Dsc;
				X_ERROR("AsyncFile", "Failed to wait for cancelled async operation to finsihed. Error: %s", core::lastError::ToString(Dsc));


			}
		}
	}
	else
	{
		core::lastError::Description Dsc;
		X_ERROR("AsyncFile", "Failed to cancel the async request. Error: %s", core::lastError::ToString(Dsc));
	}

}



X_NAMESPACE_END