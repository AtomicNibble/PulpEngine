#include "stdafx.h"
#include "OsFile.h"
#include "OsFileModeFlags.h"

#include <String\HumanSize.h>

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
#include "FileSys/xFileSys.h"
#endif

#include <WinIoCtl.h>

X_NAMESPACE_BEGIN(core)

#if X_ENABLE_FILE_STATS
XFileStats OsFile::s_stats;
#endif // !X_ENABLE_FILE_STATS

namespace
{

    void logFileError(const wchar_t* path, IFileSys::FileFlags mode)
    {
        lastError::Description Dsc;
        IFileSys::FileFlags::Description DscFlag;
        {
            X_LOG_BULLET;
            X_ERROR("File", "Failed to open file. Error: %s", lastError::ToString(Dsc));
            X_ERROR("File", "File: %ls", path);
            X_ERROR("File", "Mode: %s", mode.ToString(DscFlag));
        }
    }

    HANDLE createFileHelper(const wchar_t* path, IFileSys::FileFlags mode)
    {
        DWORD access = mode::GetAccess(mode);
        DWORD share = mode::GetShareMode(mode);
        DWORD dispo = mode::GetCreationDispo(mode);
        DWORD flags = mode::GetFlagsAndAtt(mode, false);

        return CreateFileW(path, access, share, NULL, dispo, flags, NULL);
    }

} // namespace

OsFile::OsFile(const core::Path<wchar_t>& path, IFileSys::FileFlags mode) :
    mode_(mode),
    file_(INVALID_HANDLE_VALUE)
{
    // lets open you up.
    file_ = createFileHelper(path.c_str(), mode);

#if X_ENABLE_FILE_ARTIFICAIL_DELAY

    const auto* pFileSys = static_cast<const xFileSys*>(gEnv->pFileSys);
    int32_t delay = pFileSys->getVars().artOpenDelay_;
    if (delay) {
        core::Thread::sleep(delay);
    }

#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    if (!valid()) {
        logFileError(path.c_str(), mode);
    }
    else {
#if X_ENABLE_FILE_STATS
        ++s_stats.NumFilesOpened;
#endif // !X_ENABLE_FILE_STATS

        if (mode.IsSet(IFileSys::FileFlags::APPEND)) {
            this->seek(0, SeekMode::END, false);
        }
    }
}

OsFile::~OsFile(void)
{
    if (valid()) {
        CloseHandle(file_);
    }
}

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------

size_t OsFile::read(void* pBuffer, size_t length)
{
    if (!mode_.IsSet(FileFlag::READ)) {
        IFileSys::FileFlags::Description Dsc;
        X_ERROR("File", "can't read from file. Flags: %s", mode_.ToString(Dsc));
        return 0;
    }

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a read request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return 0;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    DWORD NumRead = 0;
    if (::ReadFile(file_, pBuffer, length32, &NumRead, 0)) {
#if X_ENABLE_FILE_STATS
        s_stats.NumBytesRead += NumRead;
        ++s_stats.NumReads;
#endif // !X_ENABLE_FILE_STATS

#if X_ENABLE_FILE_ARTIFICAIL_DELAY

        const auto* pFileSys = static_cast<const xFileSys*>(gEnv->pFileSys);
        int32_t delay = pFileSys->getVars().artReadDelay_;
        if (delay) {
            core::Thread::sleep(delay);
        }

#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

        return NumRead;
    }
    else {
        lastError::Description dsc;
        X_ERROR("File", "Failed to read %d bytes from a file. Error: %s", length, lastError::ToString(dsc));
    }
    return 0;
}

size_t OsFile::write(const void* pBuffer, size_t length)
{
    if (!mode_.IsSet(FileFlag::WRITE)) {
        IFileSys::FileFlags::Description Dsc;
        X_ERROR("File", "can't write to file. Flags: %s", mode_.ToString(Dsc));
        return 0;
    }

    if (length > std::numeric_limits<uint32_t>::max()) {
        core::HumanSize::Str humanStr;
        X_ERROR("AsyncFile", "Can't make a write request bigger than 4gb. requested size: %s", core::HumanSize::toString(humanStr, length));
        return 0;
    }

    uint32_t length32 = safe_static_cast<uint32_t, size_t>(length);

    DWORD NumWrite = 0;
    if (::WriteFile(file_, pBuffer, length32, &NumWrite, 0)) {
#if X_ENABLE_FILE_STATS
        s_stats.NumBytesWrite += NumWrite;
        ++s_stats.NumWrties;
#endif // !X_ENABLE_FILE_STATS

#if X_ENABLE_FILE_ARTIFICAIL_DELAY

        const auto* pFileSys = static_cast<const xFileSys*>(gEnv->pFileSys);
        int32_t delay = pFileSys->getVars().artWriteDelay_;
        if (delay) {
            core::Thread::sleep(delay);
        }

#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

        return NumWrite;
    }
    else {
        lastError::Description dsc;
        X_ERROR("File", "Failed to read %d bytes from a file. Error: %s", NumWrite, lastError::ToString(dsc));
    }
    return 0;
}

void OsFile::seek(int64_t position, IFileSys::SeekMode::Enum origin, bool requireRandomAccess)
{
    // seeking is allowed by win32 when RANDOM_ACCESS is not passed.
    // but i want to prevent a user trying to seek if they did not use RANDOM_ACCESS.
    if (requireRandomAccess && !mode_.IsSet(FileFlag::RANDOM_ACCESS)) {
        IFileSys::FileFlags::Description Dsc;
        X_ERROR("File", "can't seek in file, requires random access. Flags: %s", mode_.ToString(Dsc));
        return;
    }

#if X_ENABLE_FILE_STATS
    s_stats.NumSeeks++;
#endif // !X_ENABLE_FILE_STATS

    LARGE_INTEGER move;
    move.QuadPart = position;

    if (!SetFilePointerEx(file_, move, 0, mode::GetSeekValue(origin))) {
        lastError::Description dsc;
        X_ERROR("File", "Failed to seek to position %d, mode %d. Error: %s", position, origin, lastError::ToString(dsc));
    }
}

uint64_t OsFile::tell(void) const
{
    LARGE_INTEGER Move, current;
    Move.QuadPart = 0;

#if X_ENABLE_FILE_STATS
    s_stats.NumTells++;
#endif // !X_ENABLE_FILE_STATS

    if (!SetFilePointerEx(file_, Move, &current, FILE_CURRENT)) {
        lastError::Description dsc;
        X_ERROR("File", "Failed to tell() file. Error: %s", lastError::ToString(dsc));
    }

    return current.QuadPart;
}

uint64_t OsFile::remainingBytes(void) const
{
#if X_ENABLE_FILE_STATS
    s_stats.NumByteLeftChecks++;
#endif // !X_ENABLE_FILE_STATS

    uint64_t size = fileSize();
    uint64_t offset = tell();
    X_ASSERT(size >= offset, "File offset is larger than file size")(size, offset);
    return size - offset;
}

uint64_t OsFile::fileSize(void) const
{
    _BY_HANDLE_FILE_INFORMATION info;

    if (!GetFileInformationByHandle(file_, &info)) {
        lastError::Description dsc;
        X_ERROR("File", "Can't Get file information. Error: %s", lastError::ToString(dsc));
    }

    uint64_t fileSize = (static_cast<uint64_t>(info.nFileSizeHigh) << 32) | static_cast<uint64_t>(info.nFileSizeLow);
    return fileSize;
}

void OsFile::setSize(int64_t numBytes)
{
    uint64_t currentPos = tell();

    seek(numBytes, SeekMode::SET, false);

    if (!::SetEndOfFile(file_)) {
        lastError::Description dsc;
        X_ERROR("File", "Failed to setSize: %Iu. Error: %s", numBytes, lastError::ToString(dsc));
    }

    seek(currentPos, SeekMode::SET, false);
}

bool OsFile::valid(void) const
{
    return (file_ != INVALID_HANDLE_VALUE);
}

#if X_ENABLE_FILE_STATS
XFileStats& OsFile::fileStats(void)
{
    return s_stats;
}
#endif // !X_ENABLE_FILE_STATS

bool OsFile::getDiskInfo(const wchar_t* pDevie, DiskInfo& info)
{
    core::zero_object(info);

    HANDLE hDevice = CreateFileW(pDevie,
        0,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        lastError::Description Dsc;
        {
            X_LOG_BULLET;
            X_ERROR("File", "Failed to open disk device. Error: %s", lastError::ToString(Dsc));
            X_ERROR("File", "DevicePath: %ls", pDevie);
        }
        return false;
    }

    DWORD outsize;
    STORAGE_PROPERTY_QUERY storageQuery;
    core::zero_object(storageQuery);

    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR diskAlignment;
    core::zero_object(diskAlignment);

    storageQuery.PropertyId = StorageAccessAlignmentProperty;
    storageQuery.QueryType = PropertyStandardQuery;

    if (!DeviceIoControl(hDevice,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &storageQuery,
            sizeof(storageQuery),
            &diskAlignment,
            sizeof(diskAlignment),
            &outsize,
            NULL)) {
        lastError::Description Dsc;
        X_ERROR("File", "Failed to query disk info. Error: %s", lastError::ToString(Dsc));
        X_ERROR("File", "DevicePath: %ls", pDevie);
        return false;
    }

    info.logicalSectorSize = diskAlignment.BytesPerLogicalSector;
    info.physicalSectorSize = diskAlignment.BytesPerPhysicalSector;
    info.cacheLaneSize = diskAlignment.BytesPerCacheLine;
    return true;
}

X_NAMESPACE_END