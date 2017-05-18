#include "stdafx.h"
#include "DiskFileAsync.h"

X_NAMESPACE_BEGIN(core)



XDiskFileAsync::XDiskFileAsync(const wchar_t* path, IFileSys::fileModeFlags mode) :
	file_(path,mode)
{

}

XDiskFileAsync::~XDiskFileAsync()
{

}


XFileAsyncOperation XDiskFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position)
{
	return XFileAsyncOperation(file_.readAsync(pBuffer, length, position), pBuffer);
}

XFileAsyncOperation XDiskFileAsync::writeAsync(const void* pBuffer, size_t length, uint64_t position)
{
	return XFileAsyncOperation(file_.writeAsync(pBuffer, length, position), pBuffer);
}

// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
size_t XDiskFileAsync::WaitUntilFinished(const XFileAsyncOperation& operation)
{
	size_t numBytes = operation.waitUntilFinished();

	// could do some genral shit now we know the operation is complete :D

	return numBytes;
}

uint64_t XDiskFileAsync::remainingBytes(void) const
{
	return file_.remainingBytes();
}

void XDiskFileAsync::setSize(int64_t numBytes)
{
	return file_.setSize(numBytes);
}









X_NAMESPACE_END