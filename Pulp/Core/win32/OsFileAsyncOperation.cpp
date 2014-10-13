#include "stdafx.h"
#include "OsFileAsyncOperation.h"

#include <Util\LastError.h>


X_NAMESPACE_BEGIN(core)


XOsFileAsyncOperation::XOsFileAsyncOperation(HANDLE hFile) :
hFile_(hFile)
{

}


bool XOsFileAsyncOperation::hasFinished(void) const
{
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, (LPOVERLAPPED)&overlapped_, &bytesTransferred, false))
		return true;
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
	
	if (::GetOverlappedResult(hFile_, (LPOVERLAPPED)&overlapped_, &bytesTransferred, true))
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

	if (::CancelIoEx(hFile_, (LPOVERLAPPED)&overlapped_))
	{
		// wait for it to finish.
		if (!::GetOverlappedResult(hFile_, (LPOVERLAPPED)&overlapped_, &bytesTransferred, true))
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