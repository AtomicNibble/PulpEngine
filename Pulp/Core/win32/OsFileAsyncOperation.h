#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_

#include "Util\ReferenceCounted.h"
#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(core)

class XOsFileAsyncOperation
{
	typedef ReferenceCountedInstance<OVERLAPPED> ReferenceCountedOverlapped;

public:
	X_INLINE XOsFileAsyncOperation(MemoryArenaBase* arena, HANDLE hFile, uint64_t position);

	X_INLINE bool hasFinished(uint32_t* pNumBytes = nullptr) const;

	// Waits until the asynchronous operation has finished
	// returns the number of transferred bytes.
	X_INLINE uint32_t waitUntilFinished(void) const;

	X_INLINE void cancel(void);

	X_INLINE OVERLAPPED* getOverlapped(void);
	X_INLINE const OVERLAPPED* getOverlapped(void) const;

private:
	HANDLE hFile_;
	mutable ReferenceCountedOwner<ReferenceCountedOverlapped> overlapped_;
};


X_NAMESPACE_END

#include "OsFileAsyncOperation.inl"

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_