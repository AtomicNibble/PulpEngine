#include "stdafx.h"
#include "DiskFile.h"

X_NAMESPACE_BEGIN(core)

XDiskFile::XDiskFile(const wchar_t* pPath, IFileSys::fileModeFlags mode) :
    file_(pPath, mode)
{
}

XDiskFile::~XDiskFile()
{
}

size_t XDiskFile::read(void* pBuffer, size_t length)
{
    return file_.read(pBuffer, length);
}

size_t XDiskFile::write(const void* pBuffer, size_t length)
{
    return file_.write(pBuffer, length);
}

void XDiskFile::seek(int64_t position, SeekMode::Enum origin)
{
    file_.seek(position, origin, true);
}

uint64_t XDiskFile::remainingBytes(void) const
{
    return file_.remainingBytes();
}

uint64_t XDiskFile::tell(void) const
{
    return file_.tell();
}

void XDiskFile::setSize(int64_t numBytes)
{
    file_.setSize(numBytes);
}

X_NAMESPACE_END