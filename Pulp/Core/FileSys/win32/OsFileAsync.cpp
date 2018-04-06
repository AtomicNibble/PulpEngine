#include "stdafx.h"
#include "OsFileAsync.h"
#include "OsFileModeFlags.h"

#include <String\HumanSize.h>

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
#include "FileSys/xFileSys.h"
#endif

X_NAMESPACE_BEGIN(core)

namespace
{
#if X_ENABLE_FILE_STATS
    static XFileStats s_stats;
#endif // !X_ENABLE_FILE_STATS

    inline VOID CALLBACK s_CompiletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlap)
    {
        if (dwErrorCode == ERROR_OPERATION_ABORTED) {
            X_WARNING("AsyncFile", "Operation was aborted");
        }

        // okay so we are sexy and kinky.
        // we need to run the callback.
        XOsFileAsyncOperation::AsyncOp* pOverlap = static_cast<XOsFileAsyncOperation::AsyncOp*>(lpOverlap);
        X_ASSERT(pOverlap->callback, "Callback not valid")
        ();

        pOverlap->callback.Invoke(pOverlap, static_cast<uint32_t>(dwNumberOfBytesTransfered));
    }

    VOID CALLBACK s_CompiletionRoutineWrite(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlap)
    {
#if X_ENABLE_FILE_STATS
        s_stats.NumBytesWrite += dwNumberOfBytesTransfered;
        ++s_stats.NumReads;
#endif // !X_ENABLE_FILE_STATS

        s_CompiletionRoutine(dwErrorCode, dwNumberOfBytesTransfered, lpOverlap);
    }

    VOID CALLBACK s_CompiletionRoutineRead(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlap)
    {
#if X_ENABLE_FILE_STATS
        s_stats.NumBytesRead += dwNumberOfBytesTransfered;
        ++s_stats.NumReads;
#endif // !X_ENABLE_FILE_STATS

        s_CompiletionRoutine(dwErrorCode, dwNumberOfBytesTransfered, lpOverlap);
    }

} // namespace

OsFileAsync::OsFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode, core::MemoryArenaBase* overlappedArena) :
    overlappedArena_(overlappedArena),
    mode_(mode),
    hFile_(INVALID_HANDLE_VALUE)
{
    X_ASSERT_NOT_NULL(path);

    DWORD access = mode::GetAccess(mode);
    DWORD share = mode::GetShareMode(mode);
    DWORD dispo = mode::GetCreationDispo(mode);
    DWORD flags = mode::GetFlagsAndAtt(mode, true);

    // lets open you up.
    hFile_ = CreateFileW(path, access, share, NULL, dispo, flags, NULL);

#if X_ENABLE_FILE_ARTIFICAIL_DELAY

    const auto* pFileSys = static_cast<const xFileSys*>(gEnv->pFileSys);
    int32_t delay = pFileSys->getVars().artOpenDelay_;
    if (delay) {
        core::Thread::Sleep(delay);
    }

#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    if (!valid()) {
        lastError::Description Dsc;
        IFileSys::fileModeFlags::Description DscFlag;
        {
            X_LOG_BULLET;
            X_ERROR("AsyncFile", "Failed to open file. Error: %s", lastError::ToString(Dsc));
            X_ERROR("AsyncFile", "File: %ls", path);
            X_ERROR("AsyncFile", "Mode: %s", mode.ToString(DscFlag));
        }
    }
    else {
#if X_ENABLE_FILE_STATS
        s_stats.NumFilesOpened++;
#endif // !X_ENABLE_FILE_STATS
    }
}

OsFileAsync::~OsFileAsync(void)
{
    if (valid()) {
        CloseHandle(hFile_);
    }
}

XOsFileAsyncOperation OsFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position)
{
    XOsFileAsyncOperation op(overlappedArena_, hFile_, position);

    LPOVERLAPPED pOverlapped = op.getOverlapped();
    LARGE_INTEGER large;
    large.QuadPart = position;

    pOverlapped->Offset = large.LowPart;
    pOverlapped->OffsetHigh = large.HighPart;

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a read request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return op;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    if (!::ReadFile(hFile_, pBuffer, length32, nullptr, op.getOverlapped())) {
        auto err = lastError::Get();
        if (err != ERROR_IO_PENDING) {
            lastError::Description dsc;
            X_ERROR("AsyncFile", "Failed to read %d bytes, position: %d from a file. Error: %s",
                length, position, lastError::ToString(err, dsc));
        }
    }

#if X_ENABLE_FILE_STATS
    s_stats.NumBytesRead += length;
    ++s_stats.NumWrties;
#endif // !X_ENABLE_FILE_STATS

    return op;
}

XOsFileAsyncOperation OsFileAsync::writeAsync(const void* pBuffer, size_t length, uint64_t position)
{
    XOsFileAsyncOperation op(overlappedArena_, hFile_, position);

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a write request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return op;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    if (!::WriteFile(hFile_, pBuffer, length32, nullptr, op.getOverlapped())) {
        auto err = lastError::Get();
        if (err != ERROR_IO_PENDING) {
            lastError::Description dsc;
            X_ERROR("AsyncFile", "Failed to write %d bytes, position: %d to a file. Error: %s",
                length, position, lastError::ToString(err, dsc));
        }
    }

    // add to stats now, since I don't have a good way to collect once requet finishes.
    // if do it in the XOsFileAsyncOperation, could double count values.
#if X_ENABLE_FILE_STATS
    s_stats.NumBytesWrite += length;
    ++s_stats.NumWrties;
#endif // !X_ENABLE_FILE_STATS

    return op;
}

XOsFileAsyncOperationCompiltion OsFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position,
    XOsFileAsyncOperation::ComplitionRotinue callBack)
{
    XOsFileAsyncOperationCompiltion op(overlappedArena_, hFile_, position, callBack);
    auto* pOverlapped = op.getOverlapped();

    LARGE_INTEGER large;
    large.QuadPart = position;

    pOverlapped->Offset = large.LowPart;
    pOverlapped->OffsetHigh = large.HighPart;

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a read request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return op;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    if (!::ReadFileEx(hFile_, pBuffer, length32, pOverlapped, s_CompiletionRoutineRead)) {
        auto err = lastError::Get();
        if (err != ERROR_IO_PENDING) {
            lastError::Description dsc;
            X_ERROR("AsyncFile", "Failed to read %d bytes, position: %d from a file. Error: %s",
                length, position, lastError::ToString(err, dsc));
        }
    }

    return op;
}

XOsFileAsyncOperationCompiltion OsFileAsync::writeAsync(void* pBuffer, size_t length, uint64_t position, XOsFileAsyncOperation::ComplitionRotinue callBack)
{
    XOsFileAsyncOperationCompiltion op(overlappedArena_, hFile_, position, callBack);

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a write request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return op;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    if (!::WriteFileEx(hFile_, pBuffer, length32, op.getOverlapped(), s_CompiletionRoutineWrite)) {
        auto err = lastError::Get();
        if (err != ERROR_IO_PENDING) {
            lastError::Description dsc;
            X_ERROR("AsyncFile", "Failed to write %d bytes, position: %d to a file. Error: %s",
                length, position, lastError::ToString(err, dsc));
        }
    }

    return op;
}

void OsFileAsync::cancelAll(void) const
{
    if (!::CancelIoEx(hFile_, nullptr)) {
        core::lastError::Description Dsc;
        X_ERROR("AsyncFile", "Failed to cancel all async requests. Error: %s", core::lastError::ToString(Dsc));
    }
}

uint64_t OsFileAsync::tell(void) const
{
    LARGE_INTEGER Move, current;
    Move.QuadPart = 0;

#if X_ENABLE_FILE_STATS
    s_stats.NumTells++;
#endif // !X_ENABLE_FILE_STATS

    if (!SetFilePointerEx(hFile_, Move, &current, FILE_CURRENT)) {
        lastError::Description dsc;
        X_ERROR("AsyncFile", "Failed to tell() file. Error: %s", lastError::ToString(dsc));
    }

    return current.QuadPart;
}

uint64_t OsFileAsync::fileSize(void) const
{
    _BY_HANDLE_FILE_INFORMATION info;

#if X_ENABLE_FILE_STATS
    s_stats.NumByteLeftChecks++;
#endif // !X_ENABLE_FILE_STATS

    if (!GetFileInformationByHandle(hFile_, &info)) {
        lastError::Description dsc;
        X_ERROR("AsyncFile", "Can't Get file information. Error: %s", lastError::ToString(dsc));
    }

    uint64_t fileSize = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | static_cast<uint64_t>(info.nFileSizeLow);
    uint64_t offset = tell();
    X_ASSERT(fileSize >= offset, "File offset is larger than file size")
    (fileSize, offset);
    return fileSize - offset;
}

void OsFileAsync::setSize(int64_t numBytes)
{
    uint64_t currentPos = tell();

    seek(numBytes, SeekMode::SET);

    if (!::SetEndOfFile(hFile_)) {
        lastError::Description dsc;
        X_ERROR("AsyncFile", "Failed to setSize: %Iu. Error: %s", numBytes, lastError::ToString(dsc));
    }

    seek(currentPos, SeekMode::SET);
}

bool OsFileAsync::valid(void) const
{
    return (hFile_ != INVALID_HANDLE_VALUE);
}

#if X_ENABLE_FILE_STATS
XFileStats& OsFileAsync::fileStats(void)
{
    return s_stats;
}
#endif // !X_ENABLE_FILE_STATS

void OsFileAsync::seek(int64_t position, IFileSys::SeekMode::Enum origin)
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

    if (!SetFilePointerEx(hFile_, move, 0, mode::GetSeekValue(origin))) {
        lastError::Description dsc;
        X_ERROR("AsyncFile", "Failed to seek to position %d, mode %d. Error: %s", position, origin, lastError::ToString(dsc));
    }
}

X_NAMESPACE_END