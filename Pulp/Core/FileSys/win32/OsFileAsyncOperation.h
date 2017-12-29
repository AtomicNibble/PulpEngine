#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_

#include "Util\ReferenceCounted.h"
#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(core)


class XOsFileAsyncOperationBase
{
public:
	struct MyOVERLAPPED;
	
	typedef core::Delegate<void(MyOVERLAPPED*, uint32_t)> ComplitionRotinue;

	struct MyOVERLAPPED : public OVERLAPPED, public ReferenceCounted<>
	{
		// we need callback to call.
		ComplitionRotinue callback;
	};

public:
	typedef MyOVERLAPPED AsyncOp;

public:
	X_INLINE XOsFileAsyncOperationBase(MemoryArenaBase* arena, uint32_t numBytes);
	X_INLINE XOsFileAsyncOperationBase(MemoryArenaBase* arena, HANDLE hFile, uint64_t position);
	XOsFileAsyncOperationBase(const XOsFileAsyncOperationBase& oth) = default;
	X_INLINE XOsFileAsyncOperationBase(XOsFileAsyncOperationBase&& oth);

	XOsFileAsyncOperationBase& operator=(const XOsFileAsyncOperationBase& oth) = default;
	X_INLINE XOsFileAsyncOperationBase& operator=(XOsFileAsyncOperationBase&& oth);

	X_INLINE bool operator==(const XOsFileAsyncOperationBase& oth) const;
	X_INLINE bool ownsAsyncOp(const AsyncOp* pOp) const;

	X_INLINE void cancel(void);

	X_INLINE bool hasFinished(uint32_t* pNumBytes = nullptr) const;

	X_INLINE AsyncOp* getOverlapped(void);
	X_INLINE const AsyncOp* getOverlapped(void) const;

protected:
	X_INLINE bool isFakeHandle(void) const;

protected:
	HANDLE hFile_;
	mutable ReferenceCountedOwner<AsyncOp> overlapped_;
};

class XOsFileAsyncOperationCompiltion : public XOsFileAsyncOperationBase
{
public:
	X_INLINE XOsFileAsyncOperationCompiltion(MemoryArenaBase* arena, HANDLE hFile, uint64_t position, ComplitionRotinue callBack);

};

class XOsFileAsyncOperation : public XOsFileAsyncOperationBase
{
public:
	using XOsFileAsyncOperationBase::XOsFileAsyncOperationBase;

	// Waits until the asynchronous operation has finished
	// returns the number of transferred bytes.
	X_INLINE uint32_t waitUntilFinished(void) const;
};



X_NAMESPACE_END

#include "OsFileAsyncOperation.inl"

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_