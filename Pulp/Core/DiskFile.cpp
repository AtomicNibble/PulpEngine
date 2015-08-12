#include "stdafx.h"
#include "DiskFile.h"


X_NAMESPACE_BEGIN(core)

XDiskFile::XDiskFile(const wchar_t* path, IFileSys::fileModeFlags mode) :
file_(path, mode)
{

}

XDiskFile::~XDiskFile()
{

}

uint32_t XDiskFile::read(void* buffer, uint32_t length)
{
	return file_.read(buffer, length);
}

uint32_t XDiskFile::write(const void* buffer, uint32_t length)
{
	return file_.write(buffer, length);
}


void XDiskFile::seek(size_t position, SeekMode::Enum origin)
{
	return file_.seek(position, origin);
}

size_t XDiskFile::remainingBytes(void) const
{
	return file_.remainingBytes();
}

size_t XDiskFile::tell(void) const
{
	return file_.tell();
}

void XDiskFile::setSize(size_t numBytes)
{
	file_.setSize(numBytes);
}




X_NAMESPACE_END