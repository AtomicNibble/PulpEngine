#include "stdafx.h"
#include "DiskFileAsync.h"

X_NAMESPACE_BEGIN(core)

XDiskFileAsync::XDiskFileAsync(const core::Path<wchar_t>& path, IFileSys::fileModeFlags mode, core::MemoryArenaBase* asyncOpArena) :
    file_(path, mode, asyncOpArena)
{
}

XDiskFileAsync::~XDiskFileAsync()
{
}

XFileAsyncOperation XDiskFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position)
{
    return file_.readAsync(pBuffer, length, position);
}

XFileAsyncOperation XDiskFileAsync::writeAsync(const void* pBuffer, size_t length, uint64_t position)
{
    return file_.writeAsync(pBuffer, length, position);
}

XFileAsyncOperationCompiltion XDiskFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack)
{
    return file_.readAsync(pBuffer, length, position, callBack);
}

XFileAsyncOperationCompiltion XDiskFileAsync::writeAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack)
{
    return file_.writeAsync(pBuffer, length, position, callBack);
}

uint64_t XDiskFileAsync::fileSize(void) const
{
    return file_.fileSize();
}

void XDiskFileAsync::setSize(int64_t numBytes)
{
    return file_.setSize(numBytes);
}

X_NAMESPACE_END