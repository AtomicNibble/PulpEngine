#include "stdafx.h"
#include "PakFileAsync.h"

#include "xFileSys.h"

X_NAMESPACE_BEGIN(core)

#if 1

XPakFileAsync::XPakFileAsync(const Pak* pPack, const AssetPak::APakEntry& entry, core::MemoryArenaBase* asyncOpArena) :
	pPack_(pPack),
	entry_(entry)
{
	X_UNUSED(asyncOpArena);
}

XPakFileAsync::~XPakFileAsync()
{

}

XFileAsyncOperation XPakFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position)
{
	uint64_t pakPos = getPosition(position);
	return pPack_->pFile->readAsync(pBuffer, length, pakPos);
}

XFileAsyncOperation XPakFileAsync::writeAsync(const void* pBuffer, size_t length, uint64_t position)
{
	// not allowed
	X_UNUSED(pBuffer, length, position);
	X_ASSERT_UNREACHABLE();

	uint64_t pakPos = getPosition(position);
	return pPack_->pFile->writeAsync(pBuffer, length, pakPos);
}

XFileAsyncOperationCompiltion XPakFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack)
{
	uint64_t pakPos = getPosition(position);	
	return pPack_->pFile->readAsync(pBuffer, length, pakPos, callBack);
}

XFileAsyncOperationCompiltion XPakFileAsync::writeAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack)
{
	// not allowed
	X_UNUSED(pBuffer, length, position, callBack);
	X_ASSERT_UNREACHABLE();

	uint64_t pakPos = getPosition(position);
	return pPack_->pFile->writeAsync(pBuffer, length, pakPos, callBack);
}

uint64_t XPakFileAsync::fileSize(void) const
{
	return entry_.inflatedSize;
}

void XPakFileAsync::setSize(int64_t numBytes)
{
	X_UNUSED(numBytes);
	X_ASSERT_NOT_IMPLEMENTED();
}


bool XPakFileAsync::valid(void) const
{
	return true;
}

void XPakFileAsync::cancelAll(void) const
{
	X_ASSERT_NOT_IMPLEMENTED();
}

size_t XPakFileAsync::waitUntilFinished(const XFileAsyncOperation& operation)
{
	return operation.waitUntilFinished();
}


X_INLINE uint64_t XPakFileAsync::getPosition(uint64_t pos) const
{
	return pPack_->dataOffset + entry_.offset + pos;
}

#endif

X_NAMESPACE_END
