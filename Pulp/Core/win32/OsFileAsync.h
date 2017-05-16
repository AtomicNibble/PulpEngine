#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_H_

#include <IFileSys.h>
#include <IFileSysStats.h>

#include "OsFileAsyncOperation.h"

X_NAMESPACE_BEGIN(core)

struct OsFileAsync
{
	OsFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode);
	~OsFileAsync(void);

	XOsFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position);
	XOsFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position);

	uint64_t tell(void) const;
	uint64_t remainingBytes(void) const;
	void setSize(int64_t numBytes);

	bool valid(void) const;

#if X_ENABLE_FILE_STATS
	static XFileStats& fileStats(void);
#endif // !X_ENABLE_FILE_STATS
private:
	void seek(int64_t position, IFileSys::SeekMode::Enum origin);

private:
	IFileSys::fileModeFlags mode_;
	HANDLE file_;

#if X_ENABLE_FILE_STATS
	static XFileStats s_stats;
#endif // !X_ENABLE_FILE_STATS
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_H_
