

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


#if X_64
X_INLINE bool XOsFileAsyncOperation::hasFinished(size_t* pNumBytes) const
{
	uint32_t numbytes = 0;
	bool res = hasFinished(&numbytes);

	if (pNumBytes) {
		*pNumBytes = numbytes;
	}

	return res;
}
#endif // !X_64

X_INLINE bool XOsFileAsyncOperation::hasFinished(uint32_t* pNumBytes) const
{
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_->instance(), &bytesTransferred, false)) {
		if (pNumBytes) {
			*pNumBytes = static_cast<size_t>(bytesTransferred);
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

X_INLINE size_t XOsFileAsyncOperation::waitUntilFinished(void) const
{
	// same as above but with bWait = true;
	DWORD bytesTransferred = 0;
	if (::GetOverlappedResult(hFile_, overlapped_->instance(), &bytesTransferred, true))
	{
		return safe_static_cast<size_t, DWORD>(bytesTransferred);
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



X_NAMESPACE_END