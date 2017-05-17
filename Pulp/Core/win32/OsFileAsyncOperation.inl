

#include <Util\LastError.h>


X_NAMESPACE_BEGIN(core)

X_INLINE XOsFileAsyncOperation::XOsFileAsyncOperation(MemoryArenaBase* arena, HANDLE hFile, uint64_t position) :
	hFile_(hFile),
	overlapped_(X_NEW(ReferenceCountedOverlapped, arena, "OVERLAPPED"), arena)
{
	LPOVERLAPPED pOverlapped = getOverlapped();
	core::zero_this(pOverlapped);

	pOverlapped->Offset = safe_static_cast<DWORD>(position & 0xFFFFFFFF);
	pOverlapped->OffsetHigh = safe_static_cast<DWORD>(((position >> 32) & 0xFFFFFFFF));
}

X_INLINE bool XOsFileAsyncOperation::hasFinished(uint32_t* pNumBytes) const
{
	// early out with fast check.
	if (!HasOverlappedIoCompleted(overlapped_->instance())) {
		return false;
	}

	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_->instance(), &bytesTransferred, false)) {
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

X_INLINE uint32_t XOsFileAsyncOperation::waitUntilFinished(void) const
{
	// same as above but with bWait = true;
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_->instance(), &bytesTransferred, true))
	{
		return safe_static_cast<uint32_t>(bytesTransferred);
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

X_INLINE void XOsFileAsyncOperation::cancel(void)
{
	DWORD bytesTransferred = 0;

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


X_INLINE OVERLAPPED* XOsFileAsyncOperation::getOverlapped(void)
{
	return overlapped_->instance();
}

X_INLINE const OVERLAPPED* XOsFileAsyncOperation::getOverlapped(void) const
{
	return overlapped_->instance();
}


X_NAMESPACE_END