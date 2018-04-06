#pragma once

#ifndef _X_FILE_SYSTEM_OSFILE_ASYNC_H_
#define _X_FILE_SYSTEM_OSFILE_ASYNC_H_

#include <IFileSys.h>
#include <IFileSysStats.h>

#include "OsFileAsyncOperation.h"

X_NAMESPACE_BEGIN(core)

class OsFileAsync
{
public:
    OsFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode, core::MemoryArenaBase* overlappedArena);
    ~OsFileAsync(void);

    XOsFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position);
    XOsFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position);

    XOsFileAsyncOperationCompiltion readAsync(void* pBuffer, size_t length, uint64_t position, XOsFileAsyncOperation::ComplitionRotinue callBack);
    XOsFileAsyncOperationCompiltion writeAsync(void* pBuffer, size_t length, uint64_t position, XOsFileAsyncOperation::ComplitionRotinue callBack);

    void cancelAll(void) const;

    uint64_t tell(void) const;
    uint64_t fileSize(void) const;
    void setSize(int64_t numBytes);

    bool valid(void) const;

#if X_ENABLE_FILE_STATS
    static XFileStats& fileStats(void);
#endif // !X_ENABLE_FILE_STATS

private:
    void seek(int64_t position, IFileSys::SeekMode::Enum origin);

private:
    core::MemoryArenaBase* overlappedArena_;
    IFileSys::fileModeFlags mode_;
    HANDLE hFile_;
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_OSFILE_ASYNC_H_
