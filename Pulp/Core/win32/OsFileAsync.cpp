#include "stdafx.h"
#include "OsFileAsync.h"
#include "OsFileModeFlags.h"


#include <String\HumanSize.h>


X_NAMESPACE_BEGIN(core)

#ifndef MAKEQWORD
#define MAKEQWORD(a,b) ((QWORD)( ((QWORD) ((DWORD) (a))) << 32 | ((DWORD) (b))))
#endif



#if X_ENABLE_FILE_STATS
XFileStats OsFileAsync::s_stats = { 0 };
#endif // !X_ENABLE_FILE_STATS


OsFileAsync::OsFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode) :
file_(INVALID_HANDLE_VALUE),
mode_(mode)
{
	X_ASSERT_NOT_NULL(path);

	DWORD access = mode::GetAccess(mode);
	DWORD share = mode::GetShareMode(mode);
	DWORD dispo = mode::GetCreationDispo(mode);
	DWORD flags = mode::GetFlagsAndAtt(mode, true);


	// lets open you up.
	file_ = CreateFileW(path, access, share, NULL, dispo, flags, NULL);


	if (!valid())
	{
		lastError::Description Dsc;
		IFileSys::fileModeFlags::Description DscFlag;
		{
			X_LOG_BULLET;
			X_ERROR("AsyncFile", "Failed top open file. Error: %s", lastError::ToString(Dsc));
			X_ERROR("AsyncFile", "File: %ls", path);
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

XOsFileAsyncOperation OsFileAsync::readAsync(void* pBuffer, size_t length, size_t position)
{
	XOsFileAsyncOperation op(gEnv->pArena, file_, position);

	LPOVERLAPPED pOverlapped = op.getOverlapped();
	LARGE_INTEGER large;
	large.QuadPart = position;

	pOverlapped->Offset = large.LowPart;
	pOverlapped->OffsetHigh = large.HighPart;

	uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

#if X_64
	if (length > std::numeric_limits<uint32_t>::max()) {
		core::HumanSize::Str humanStr;
		X_ERROR("AsyncFile", "Can't make a read request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
		return op;
	}
#endif // X_64

	if (::ReadFile(file_, pBuffer, length32, nullptr, op.getOverlapped()))
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

XOsFileAsyncOperation OsFileAsync::writeAsync(const void* pBuffer, size_t length, size_t position)
{
	XOsFileAsyncOperation op(gEnv->pArena, file_, position);

	uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

#if X_64
	if (length > std::numeric_limits<uint32_t>::max()) {
		core::HumanSize::Str humanStr;
		X_ERROR("AsyncFile", "Can't make a write request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
		return op;
	}
#endif // X_64

	if (::WriteFile(file_, pBuffer, length32, nullptr, op.getOverlapped()))
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
		X_ERROR("AsyncFile", "Failed to tell() file. Error: %s", lastError::ToString(dsc));
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
		X_ERROR("AsyncFile", "Can't Get file information. Error: %s", lastError::ToString(dsc));
	}

#if X_64 == 1
	return MAKEQWORD(info.nFileSizeHigh, info.nFileSizeLow) - tell();
#else
	X_ASSERT(info.nFileSizeHigh == 0, "tell was called on a file larger than 1 << 32 not supported in 32bit build")(info.nFileSizeHigh);
	return info.nFileSizeLow - tell();
#endif
}

void OsFileAsync::setSize(size_t numBytes)
{
	size_t currentPos = tell();

	seek(numBytes, SeekMode::SET);

	if (!::SetEndOfFile(file_))
	{
		lastError::Description dsc;
		X_ERROR("AsyncFile", "Failed to setSize: %Iu. Error: %s", numBytes, lastError::ToString(dsc));
	}

	seek(currentPos, SeekMode::SET);
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

void OsFileAsync::seek(size_t position, IFileSys::SeekMode::Enum origin)
{
	// is this condition correct?
	if (!mode_.IsSet(fileMode::RANDOM_ACCESS) && !mode_.IsSet(fileMode::APPEND)) {
		IFileSys::fileModeFlags::Description Dsc;
		X_ERROR("AsyncFile", "can't seek in file, requires random access. Flags: %s", mode_.ToString(Dsc));
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
		X_ERROR("AsyncFile", "Failed to seek to position %d, mode %d. Error: %s", position, origin, lastError::ToString(dsc));
	}
}

X_NAMESPACE_END