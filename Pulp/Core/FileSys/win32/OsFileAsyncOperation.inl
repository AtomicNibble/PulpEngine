


X_NAMESPACE_BEGIN(core)

X_INLINE XOsFileAsyncOperationBase::XOsFileAsyncOperationBase(MemoryArenaBase* arena, uint32_t numBytes) :
	hFile_(INVALID_HANDLE_VALUE),
	overlapped_(X_NEW(MyOVERLAPPED, arena, "OVERLAPPED"), arena)
{
	auto* pOverlapped = getOverlapped();
	core::zero_this<OVERLAPPED>(pOverlapped);

	pOverlapped->Internal = numBytes;
}

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

X_INLINE XOsFileAsyncOperationBase::AsyncOp* XOsFileAsyncOperationBase::getOverlapped(void)
{
	return overlapped_.instance();
}

X_INLINE const XOsFileAsyncOperationBase::AsyncOp* XOsFileAsyncOperationBase::getOverlapped(void) const
{
	return overlapped_.instance();
}

X_INLINE bool XOsFileAsyncOperationBase::isFakeHandle(void) const
{
	return hFile_ == INVALID_HANDLE_VALUE;
}


// --------------------------------------------------------------------

X_INLINE XOsFileAsyncOperationCompiltion::XOsFileAsyncOperationCompiltion(MemoryArenaBase* arena, 
	HANDLE hFile, uint64_t position, ComplitionRotinue callBack) :
	XOsFileAsyncOperationBase(arena, hFile, position)
{
	auto* pOverlapped = getOverlapped();
	pOverlapped->callback = callBack;
}


X_NAMESPACE_END