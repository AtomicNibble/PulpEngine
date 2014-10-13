#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_


X_NAMESPACE_BEGIN(core)

struct XOsFileAsyncOperation
{
	XOsFileAsyncOperation(HANDLE hFile);

public:
	bool hasFinished(void) const;

	// Waits until the asynchronous operation has finished
	// returns the number of transferred bytes.
	uint32_t waitUntilFinished(void) const;

	void cancel(void);


	X_INLINE OVERLAPPED* getOverlapped(void) {
		return &overlapped_;
	}
	X_INLINE const OVERLAPPED* getOverlapped(void) const {
		return &overlapped_;
	}
private:
	HANDLE hFile_;
	OVERLAPPED overlapped_;
};


X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_OPERATION_H_