#pragma once

#ifndef X_CORE_FILESYS_DISK_FILE_ASYNC_H_
#define X_CORE_FILESYS_DISK_FILE_ASYNC_H_

#include <IFileSys.h>

#include X_INCLUDE(X_PLATFORM/OsFileAsync.h)

X_NAMESPACE_BEGIN(core)




class XDiskFileAsync : public XFileAsync
{
public:
	XDiskFileAsync(const char* path, IFileSys::fileModeFlags mode);
	~XDiskFileAsync() X_FINAL;


	/// Returns whether the disk file is valid.
	inline bool valid(void) const {
		return file_.valid();
	}

	XFileAsyncOperation readAsync(void* pBuffer, uint32_t length, uint32_t position) X_FINAL;

	// Asynchronously writes from a buffer into the file.
	XFileAsyncOperation writeAsync(const void* pBuffer, uint32_t length, uint32_t position) X_FINAL;

	// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
	uint32_t WaitUntilFinished(const XFileAsyncOperation& operation) X_FINAL;
	
	size_t remainingBytes(void) const X_FINAL;

	void setSize(size_t numBytes) X_FINAL;

private:

	OsFileAsync file_;
};

X_NAMESPACE_END



#endif // !X_CORE_FILESYS_DISK_FILE_ASYNC_H_