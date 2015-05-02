#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_

#include "Util\ReferenceCounted.h"
#include "Util\ReferenceCountedOwner.h"

X_NAMESPACE_BEGIN(core)

struct XOsFileAsyncOperation
{
	struct ReferenceCountedOverlapped : public OVERLAPPED, public
		core::ReferenceCounted<ReferenceCountedOverlapped>
	{

	};


public:
	XOsFileAsyncOperation(MemoryArenaBase* arena, HANDLE hFile, size_t position);

	bool hasFinished(uint32_t* pNumBytes = nullptr) const;

	// Waits until the asynchronous operation has finished
	// returns the number of transferred bytes.
	uint32_t waitUntilFinished(void) const;

	void cancel(void);


	X_INLINE OVERLAPPED* getOverlapped(void) {
		return overlapped_.instance();
	}
	X_INLINE const OVERLAPPED* getOverlapped(void) const {
		return overlapped_.instance();
	}
private:
	HANDLE hFile_;
	mutable ReferenceCountedOwner<ReferenceCountedOverlapped> overlapped_;
};


X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_