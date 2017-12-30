#include "stdafx.h"
#include "OsFileAsyncOperation.h"

#include <Util\LastError.h>

#include <Threading\JobSystem2.h>


X_NAMESPACE_BEGIN(core)

XOsFileAsyncOperationBase::XOsFileAsyncOperationBase(MemoryArenaBase* arena, uint32_t numBytes, core::V2::Job* pJob) :
	hFile_(INVALID_HANDLE_VALUE),
	overlapped_(X_NEW(MyOVERLAPPED, arena, "OVERLAPPED"), arena)
{
	auto* pOverlapped = getOverlapped();
	core::zero_this<OVERLAPPED>(pOverlapped);

	pOverlapped->Internal = numBytes;
	pOverlapped->Pointer = pJob;
}


void XOsFileAsyncOperationBase::cancel(void)
{
	DWORD bytesTransferred = 0;

	if (isFakeHandle()) {
		return;
	}

	if (::CancelIoEx(hFile_, getOverlapped()))
	{
		// wait for it to finish.
		if (!::GetOverlappedResult(hFile_, getOverlapped(), &bytesTransferred, true))
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


bool XOsFileAsyncOperationBase::hasFinished(uint32_t* pNumBytes) const
{
	if (isFakeHandle()) {

		auto* pOverlapped = getOverlapped();
		if (pOverlapped->Pointer) {
			auto* pJob = reinterpret_cast<core::V2::Job*>(pOverlapped->Pointer);
			if (!core::V2::JobSystem::HasJobCompleted(pJob)) {
				return false;
			}
		}

		*pNumBytes = safe_static_cast<uint32_t>(pOverlapped->Internal);
		return true;
	}

	// early out with fast check.
	if (!HasOverlappedIoCompleted(overlapped_.instance())) {
		return false;
	}

	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_.instance(), &bytesTransferred, false)) {
		if (pNumBytes) {
			*pNumBytes = static_cast<uint32_t>(bytesTransferred);
		}
		return true;
	}

	auto err = core::lastError::Get();
	if (err == ERROR_IO_INCOMPLETE) {
		return false;
	}

	// some goaty error
	core::lastError::Description Dsc;
	X_ERROR("AsyncFile", "Failed to check if async request has finsihed. Error: %s", core::lastError::ToString(err, Dsc));
	return false;
}

// --------------------------------------------------------------------

uint32_t XOsFileAsyncOperation::waitUntilFinished(void) const
{
	if (isFakeHandle()) {

		auto* pOverlapped = getOverlapped();
		if (pOverlapped->Pointer) {
			auto* pJob = reinterpret_cast<core::V2::Job*>(pOverlapped->Pointer);
			gEnv->pJobSys->Wait(pJob);
		}

		return safe_static_cast<uint32_t>(pOverlapped->Internal);
	}

	// same as above but with bWait = true;
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_.instance(), &bytesTransferred, true)) {
		return safe_static_cast<uint32_t>(bytesTransferred);
	}

	// some goaty error
	core::lastError::Description Dsc;
	X_ERROR("AsyncFile", "Failed to wait until async request has finsihed. Error: %s", core::lastError::ToString(Dsc));

	// nope.
	return 0;
}



X_NAMESPACE_END
