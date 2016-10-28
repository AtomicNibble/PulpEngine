#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_

#include "Util\ReferenceCounted.h"
#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(core)

struct XOsFileAsyncOperation
{
	typedef ReferenceCountedInstance<OVERLAPPED> ReferenceCountedOverlapped;


public:
	XOsFileAsyncOperation(MemoryArenaBase* arena, HANDLE hFile, uint64_t position);

#if X_64
	bool hasFinished(size_t* pNumBytes = nullptr) const;
#endif // !X_64
	bool hasFinished(uint32_t* pNumBytes = nullptr) const;

	// Waits until the asynchronous operation has finished
	// returns the number of transferred bytes.
	size_t waitUntilFinished(void) const;

	void cancel(void);


	X_INLINE OVERLAPPED* getOverlapped(void) {
		return overlapped_->instance();
	}
	X_INLINE const OVERLAPPED* getOverlapped(void) const {
		return overlapped_->instance();
	}
private:
	HANDLE hFile_;
	mutable ReferenceCountedOwner<ReferenceCountedOverlapped> overlapped_;
};


X_NAMESPACE_END

#include "OsFileAsyncOperation.inl"

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_