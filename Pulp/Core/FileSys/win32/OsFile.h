#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_H_
#define _X_FILE_SYSTEM_OSFILE_H_

#include <IFileSys.h>
#include <IFileSysStats.h>

X_NAMESPACE_BEGIN(core)


class OsFile
{
public:
	struct DiskInfo
	{
		size_t physicalSectorSize;
		size_t logicalSectorSize;
		size_t cacheLaneSize;
	};

public:
	OsFile(const wchar_t* path, IFileSys::fileModeFlags mode);
	~OsFile(void);

	size_t read(void* buffer, size_t length);
	size_t write(const void* buffer, size_t length);

	void seek(int64_t position, IFileSys::SeekMode::Enum origin, bool requireRandomAccess);

	uint64_t tell(void) const;
	uint64_t remainingBytes(void) const;
	void setSize(int64_t numBytes);

	bool valid(void) const;

	static bool getDiskInfo(const wchar_t* pDevie, DiskInfo& info);


#if X_ENABLE_FILE_STATS
	static XFileStats& fileStats(void);
#endif // !X_ENABLE_FILE_STATS

private:
	IFileSys::fileModeFlags mode_;
	HANDLE file_;

#if X_ENABLE_FILE_STATS
	static XFileStats s_stats;
#endif // !X_ENABLE_FILE_STATS
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_H_
