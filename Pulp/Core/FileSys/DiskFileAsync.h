#pragma once

#ifndef X_CORE_FILESYS_DISK_FILE_ASYNC_H_
#define X_CORE_FILESYS_DISK_FILE_ASYNC_H_

#include <IFileSys.h>

#include X_INCLUDE(X_PLATFORM/OsFileAsync.h)

X_NAMESPACE_BEGIN(core)

class XDiskFileAsync : public XFileAsync
{
public:
    XDiskFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode, core::MemoryArenaBase* asyncOpArena);
    ~XDiskFileAsync() X_FINAL;

    /// Returns whether the disk file is valid.
    X_INLINE bool valid(void) const;

    Type::Enum getType(void) const X_FINAL;

    XFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position) X_FINAL;
    XFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position) X_FINAL;

    XFileAsyncOperationCompiltion readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack);
    XFileAsyncOperationCompiltion writeAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack);

    X_INLINE void cancelAll(void) const X_FINAL;

    // Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
    X_INLINE size_t waitUntilFinished(const XFileAsyncOperation& operation) X_FINAL;

    uint64_t fileSize(void) const X_FINAL;

    void setSize(int64_t numBytes) X_FINAL;

private:
    OsFileAsync file_;
};

X_INLINE XDiskFileAsync::Type::Enum XDiskFileAsync::getType(void) const
{
    return Type::DISK;
}

X_INLINE bool XDiskFileAsync::valid(void) const
{
    return file_.valid();
}

X_INLINE void XDiskFileAsync::cancelAll(void) const
{
    file_.cancelAll();
}

X_INLINE size_t XDiskFileAsync::waitUntilFinished(const XFileAsyncOperation& operation)
{
    return operation.waitUntilFinished();
}

X_NAMESPACE_END

#endif // !X_CORE_FILESYS_DISK_FILE_ASYNC_H_