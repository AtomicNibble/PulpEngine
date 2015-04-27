#include "stdafx.h"
#include "OsFileAsync.h"
#include "OsFileModeFlags.h"


X_NAMESPACE_BEGIN(core)

#ifndef MAKEQWORD
#define MAKEQWORD(a,b) ((QWORD)( ((QWORD) ((DWORD) (a))) << 32 | ((DWORD) (b))))
#endif



#if X_ENABLE_FILE_STATS
XFileStats OsFileAsync::s_stats = { 0 };
#endif // !X_ENABLE_FILE_STATS


OsFileAsync::OsFileAsync(const char* path, IFileSys::fileModeFlags mode) :
file_(INVALID_HANDLE_VALUE),
mode_(mode)
{
	X_ASSERT_NOT_NULL(path);

	DWORD access = mode::GetAccess(mode);
	DWORD share = mode::GetShareMode(mode);
	DWORD dispo = mode::GetCreationDispo(mode);
	DWORD flags = mode::GetFlagsAndAtt(mode, true);


	// lets open you up.
	file_ = CreateFile(path, access, share, NULL, dispo, flags, NULL);


	if (!valid())
	{
		lastError::Description Dsc;
		IFileSys::fileModeFlags::Description DscFlag;
		{
			X_LOG_BULLET;
			X_ERROR("AsyncFile", "Failed top open file. Error: %s", lastError::ToString(Dsc));
			X_ERROR("AsyncFile", "File: %s", path);
			X_ERROR("AsyncFile", "Mode: %s", mode.ToString(DscFlag));
		}
	}
	else 
	{
#if X_ENABLE_FILE_STATS
		s_stats.NumFilesOpened++;
#endif // !X_ENABLE_FILE_STATS
	}



}

OsFileAsync::~OsFileAsync(void)
{
	if (valid())
		CloseHandle(file_);
}

XOsFileAsyncOperation OsFileAsync::readAsync(void* pBuffer, uint32_t length, uint32_t position)
{
	XOsFileAsyncOperation op(file_);

	if (::ReadFile(file_, pBuffer, length, nullptr, op.getOverlapped()))
	{

	}
	else if (lastError::Get() != ERROR_IO_PENDING)
	{
		lastError::Description dsc;
		X_ERROR("AsyncFile", "Failed to read %d bytes, position: %d from a file. Error: %s",
			length, position, lastError::ToString(dsc));

	}

	return op;
}

XOsFileAsyncOperation OsFileAsync::writeAsync(const void* pBuffer, uint32_t length, uint32_t position)
{
	XOsFileAsyncOperation op(file_);

	if (::WriteFile(file_, pBuffer, length, nullptr, op.getOverlapped()))
	{

	}
	else if (lastError::Get() != ERROR_IO_PENDING)
	{
		lastError::Description dsc;
		X_ERROR("AsyncFile", "Failed to write %d bytes, position: %d to a file. Error: %s",
			length, position, lastError::ToString(dsc));

	}

	return op;
}


size_t OsFileAsync::tell(void) const
{
	LARGE_INTEGER Move, current;
	Move.QuadPart = 0;

#if X_ENABLE_FILE_STATS
	s_stats.NumTells++;
#endif // !X_ENABLE_FILE_STATS

	if (!SetFilePointerEx(file_, Move, &current, FILE_CURRENT))
	{
		lastError::Description dsc;
		X_ERROR("File", "Failed to tell() file. Error: %s", lastError::ToString(dsc));
	}

#if X_64 == 1
	return current.QuadPart;
#else
	return current.LowPart;
#endif
}

size_t OsFileAsync::remainingBytes(void) const
{
	_BY_HANDLE_FILE_INFORMATION info;

#if X_ENABLE_FILE_STATS
	s_stats.NumByteLeftChecks++;
#endif // !X_ENABLE_FILE_STATS

	if (!GetFileInformationByHandle(file_, &info))
	{
		lastError::Description dsc;
		X_ERROR("File", "Can't Get file information. Error: %s", lastError::ToString(dsc));
	}

#if X_64 == 1
	return MAKEQWORD(info.nFileSizeHigh, info.nFileSizeLow) - tell();
#else
	X_ASSERT(info.nFileSizeHigh == 0, "tell was called on a file larger than 1 << 32 not supported in 32bit build")(info.nFileSizeHigh);
	return info.nFileSizeLow - tell();
#endif
}

bool OsFileAsync::valid(void) const
{
	return (file_ != INVALID_HANDLE_VALUE);
}

XFileStats& OsFileAsync::fileStats(void)
{
#if X_ENABLE_FILE_STATS
	return s_stats;
#else
	static XFileStats blank;
	core::zero_object(blank);
	return blank;
#endif // !X_ENABLE_FILE_STATS
}


X_NAMESPACE_END