#include "stdafx.h"
#include "OsFile.h"
#include "OsFileModeFlags.h"

#include "IFileSys.h"

#include <String\StringUtil.h>


X_NAMESPACE_BEGIN(core)

#ifndef MAKEQWORD
#define MAKEQWORD(a,b) ((QWORD)( ((QWORD) ((DWORD) (a))) << 32 | ((DWORD) (b))))
#endif


#if X_ENABLE_FILE_STATS
XFileStats OsFile::s_stats = {0};
#endif // !X_ENABLE_FILE_STATS


OsFile::OsFile(const wchar_t* path, IFileSys::fileModeFlags mode) :
file_(INVALID_HANDLE_VALUE), 
mode_(mode)
{
	DWORD access = mode::GetAccess(mode);
	DWORD share = mode::GetShareMode(mode);
	DWORD dispo = mode::GetCreationDispo(mode);
	DWORD flags = mode::GetFlagsAndAtt(mode, false);


	// lets open you up.
	file_ = CreateFileW(path, access, share, NULL, dispo, flags, NULL);

	if (!valid())
	{
		lastError::Description Dsc;
		IFileSys::fileModeFlags::Description DscFlag;
		{
			X_LOG_BULLET;
			X_ERROR("File", "Failed top open file. Error: %s", lastError::ToString(Dsc));
			X_ERROR("File", "File: %s", path);
			X_ERROR("File", "Mode: %s", mode.ToString(DscFlag));
		}
	}
	else {
#if X_ENABLE_FILE_STATS
		s_stats.NumFilesOpened++;
#endif // !X_ENABLE_FILE_STATS

		if (mode.IsSet(IFileSys::fileModeFlags::APPEND))
			this->seek(0, SeekMode::END);
	}
}


OsFile::~OsFile(void)
{
	if (valid())
		CloseHandle(file_);
}

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------



uint32_t OsFile::read(void* buffer, uint32_t length)
{
	if (!mode_.IsSet(fileMode::READ)) {
		IFileSys::fileModeFlags::Description Dsc;
		X_ERROR("File", "can't read from file. Flags: %s", mode_.ToString(Dsc));
		return 0;
	}

	DWORD NumRead = 0;
	if (::ReadFile(file_, buffer, length, &NumRead, 0)) {
#if X_ENABLE_FILE_STATS
		s_stats.NumBytesRead += NumRead;
#endif // !X_ENABLE_FILE_STATS

		return NumRead;
	}
	else
	{
		lastError::Description dsc;
		X_ERROR("File", "Failed to read %d bytes from a file. Error: %s", length, lastError::ToString(dsc));
	}
	return 0;
}


uint32_t OsFile::write(const void* buffer, uint32_t length)
{
	if (!mode_.IsSet(fileMode::WRITE)) {
		IFileSys::fileModeFlags::Description Dsc;
		X_ERROR("File", "can't write to file. Flags: %s", mode_.ToString(Dsc));
		return 0;
	}

	DWORD NumWrite = 0;
	if (::WriteFile(file_, buffer, length, &NumWrite, 0)) {
#if X_ENABLE_FILE_STATS
		s_stats.NumBytesWrite += NumWrite;
#endif // !X_ENABLE_FILE_STATS

		return NumWrite;
	}
	else
	{
		lastError::Description dsc;
		X_ERROR("File", "Failed to read %d bytes from a file. Error: %s", NumWrite, lastError::ToString(dsc));
	}
	return 0;
}


void OsFile::seek(size_t position, IFileSys::SeekMode::Enum origin)
{
	// is this condition correct?
	if (!mode_.IsSet(fileMode::RANDOM_ACCESS) && !mode_.IsSet(fileMode::APPEND)) {
		IFileSys::fileModeFlags::Description Dsc;
		X_ERROR("File", "can't seek in file, requires random access. Flags: %s", mode_.ToString(Dsc));
		return;
	}

#if X_ENABLE_FILE_STATS
	s_stats.NumSeeks++;
#endif // !X_ENABLE_FILE_STATS

	LARGE_INTEGER move;
	move.QuadPart = position;

	if (!SetFilePointerEx(file_, move, 0, mode::GetSeekValue(origin)))
	{
		lastError::Description dsc;
		X_ERROR("File", "Failed to seek to position %d, mode %d. Error: %s", position, origin, lastError::ToString(dsc));
	}
}

size_t OsFile::tell(void) const
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


size_t OsFile::remainingBytes(void) const
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

void OsFile::setSize(size_t numBytes)
{
	size_t currentPos = tell();

	seek(numBytes, SeekMode::SET);

	if (!::SetEndOfFile(file_))
	{
		lastError::Description dsc;
		X_ERROR("File", "Failed to setSize: %Iu. Error: %s", numBytes, lastError::ToString(dsc));
	}

	seek(currentPos, SeekMode::SET);
}

bool OsFile::valid(void) const
{
	return (file_ != INVALID_HANDLE_VALUE);
}

XFileStats& OsFile::fileStats(void)
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