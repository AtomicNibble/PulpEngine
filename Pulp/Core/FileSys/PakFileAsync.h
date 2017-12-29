#pragma once

#include <IFileSys.h>
#include <IAssetPak.h>

X_NAMESPACE_BEGIN(core)

struct Pak;

/*

so for pack files the data with either be memcpy or streamed.
if we are just memcpying should we still proces async requests?
yer i think so as then the processing of the read data get's submitted to jobs still.

i just think maybe the IO thread can do the copying?
well ideally for packs in memory we just return a span.

but the asset may be compressed so we still need a buffer to place the infalted asset in.

so if the load request gives us a buffer and we have the pack in memory
we can just make a job to inflate from the pak into the request buffer
which can be hidden in a async IO op.


*/

#if 1

class XPakFileAsync : public XFileAsync
{
public:
	XPakFileAsync(const Pak* pPack, const AssetPak::APakEntry& entry, core::MemoryArenaBase* asyncOpArena);
	~XPakFileAsync() X_FINAL;

	bool valid(void) const;

	Type::Enum getType(void) const X_FINAL;

	XFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position) X_FINAL;
	XFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position) X_FINAL;

	XFileAsyncOperationCompiltion readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack) X_FINAL;
	XFileAsyncOperationCompiltion writeAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack) X_FINAL;

	void cancelAll(void) const X_FINAL;

	// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
	size_t waitUntilFinished(const XFileAsyncOperation& operation) X_FINAL;

	uint64_t fileSize(void) const X_FINAL;

	void setSize(int64_t numBytes) X_FINAL;

private:
	uint64_t getPosition(size_t length, uint64_t pos) const;

private:
	const Pak* pPack_;
	const AssetPak::APakEntry& entry_;
};

X_INLINE XPakFileAsync::Type::Enum XPakFileAsync::getType(void) const
{
	return Type::VIRTUAL;
}

#endif

X_NAMESPACE_END
