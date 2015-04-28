#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_H_

#include "IFileSys.h"

#include "OsFileAsyncOperation.h"

X_NAMESPACE_BEGIN(core)

struct OsFileAsync
{
	OsFileAsync(const char* path, IFileSys::fileModeFlags mode);
	~OsFileAsync(void);

	XOsFileAsyncOperation OsFileAsync::readAsync(void* pBuffer, uint32_t length, uint32_t position);
	XOsFileAsyncOperation OsFileAsync::writeAsync(const void* pBuffer, uint32_t length, uint32_t position);

	size_t tell(void) const;
	size_t remainingBytes(void) const;
	void setSize(size_t numBytes);

	bool valid(void) const;

	static XFileStats& fileStats(void);

private:
	void seek(size_t position, IFileSys::SeekMode::Enum origin);

private:
	IFileSys::fileModeFlags mode_;
	HANDLE file_;

	static XFileStats s_stats;
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_H_
