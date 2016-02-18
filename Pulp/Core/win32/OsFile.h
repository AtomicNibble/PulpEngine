#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_H_
#define _X_FILE_SYSTEM_OSFILE_H_

#include "IFileSys.h"

X_NAMESPACE_BEGIN(core)


struct OsFile
{
	OsFile(const wchar_t* path, IFileSys::fileModeFlags mode);
	~OsFile(void);

	size_t read(void* buffer, size_t length);
	size_t write(const void* buffer, size_t length);

	void seek(int64_t position, IFileSys::SeekMode::Enum origin);

	uint64_t tell(void) const;
	uint64_t remainingBytes(void) const;
	void setSize(int64_t numBytes);

	bool valid(void) const;

	static XFileStats& fileStats(void);

private:
	IFileSys::fileModeFlags mode_;
	HANDLE file_;

	static XFileStats s_stats;
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_H_
