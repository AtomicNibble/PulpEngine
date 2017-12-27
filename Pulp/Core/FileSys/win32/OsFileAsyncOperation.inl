

#include <Util\LastError.h>


X_NAMESPACE_BEGIN(core)

X_INLINE XOsFileAsyncOperationBase::XOsFileAsyncOperationBase(MemoryArenaBase* arena, HANDLE hFile, uint64_t position) :
	hFile_(hFile),
	overlapped_(X_NEW(MyOVERLAPPED, arena, "OVERLAPPED"), arena)
{
	auto* pOverlapped = getOverlapped();
	core::zero_this<OVERLAPPED>(pOverlapped);

	pOverlapped->Offset = safe_static_cast<DWORD>(position & 0xFFFFFFFF);
	pOverlapped->OffsetHigh = safe_static_cast<DWORD>(((position >> 32) & 0xFFFFFFFF));
}

X_INLINE XOsFileAsyncOperationBase::XOsFileAsyncOperationBase(XOsFileAsyncOperationBase&& oth) :
	hFile_(oth.hFile_),
	overlapped_(std::move(oth.overlapped_))
{

}

X_INLINE XOsFileAsyncOperationBase& XOsFileAsyncOperationBase::operator=(XOsFileAsyncOperationBase&& oth)
{
	hFile_ = std::move(oth.hFile_);
	overlapped_ = std::move(oth.overlapped_);
	return *this;
}

X_INLINE bool XOsFileAsyncOperationBase::operator==(const XOsFileAsyncOperationBase& oth) const
{
	// the overlapped instance should be enougth but check file also.
	return hFile_ == oth.hFile_ && overlapped_.instance() == oth.overlapped_.instance();
}

X_INLINE bool XOsFileAsyncOperationBase::ownsAsyncOp(const AsyncOp* pOp) const
{
	return overlapped_.instance() == pOp;
}


X_INLINE void XOsFileAsyncOperationBase::cancel(void)
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


X_INLINE XOsFileAsyncOperationBase::AsyncOp* XOsFileAsyncOperationBase::getOverlapped(void)
{
	return overlapped_.instance();
}

X_INLINE const XOsFileAsyncOperationBase::AsyncOp* XOsFileAsyncOperationBase::getOverlapped(void) const
{
	return overlapped_.instance();
}


X_INLINE bool XOsFileAsyncOperationBase::hasFinished(uint32_t* pNumBytes) const
{
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

X_INLINE XOsFileAsyncOperationCompiltion::XOsFileAsyncOperationCompiltion(MemoryArenaBase* arena, 
	HANDLE hFile, uint64_t position, ComplitionRotinue callBack) :
	XOsFileAsyncOperationBase(arena, hFile, position)
{
	auto* pOverlapped = getOverlapped();
	pOverlapped->callback = callBack;
}



// --------------------------------------------------------------------


X_INLINE uint32_t XOsFileAsyncOperation::waitUntilFinished(void) const
{
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