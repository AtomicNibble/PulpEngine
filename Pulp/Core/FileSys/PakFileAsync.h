#pragma once

#include <IFileSys.h>
#include <IFileSysStats.h>

#include <IAssetPak.h>

X_NAMESPACE_BEGIN(core)

namespace V2 {
class JobSystem;
struct Job;
}

struct Pak;

/*

Represents a file 'inside' a pak file.

The behaviour of this is diffrent for the following cases:

1. pak fully in memory - asset uncompressed.
	Reportedsize: uncompressed
	- simple memcpy to destination

2. pak fully in memory - asset compressed.
	Reportedsize: uncompressed
	- job is dispatched to inflate directly into target buffer.

3. pak on disk - asset uncompressed
	Reportedsize: uncompressed
	- normal async read is dispatched, with complition routine. (reads are just offset, into pak)

4. pak on disk - asset uncompressed
	Reportedsize: compressed
	- we say the file is the size of the compressed data, so reads are just like normal reads.
		but the user will get back a compressed buffer, and have to inflate themselves.

	Notes:
		Some of the reasons I decided not to make the compression transparent in this case.

		1. it creats a false sense of IO delay / load, as it's IO + inflation time.
			and we may actually be cpu bound not IO bound.

		2. all temp buffers would have to be allocated by file system.

		3. if compression is done outside the filesystem it's more easy for a AssetManager to
			correctly rate limit IO requests and also limit the rate of inflation jobs.

	Downsides Of this approach:

		1. anyone who reads assets needs to know they might get back a compressed file.
			- I think this will be somewhat mitigated once i've finished moving to a more unified asset loading code.
			
			- this does also mean we could compress loose assets outside pak and they should get handled fine.
				not sure I would want todo that.
    


*/

class XPakFileAsync : public XFileAsync
{
public:
	XPakFileAsync(Pak* pPack, const AssetPak::APakEntry& entry, core::MemoryArenaBase* asyncOpArena);
	~XPakFileAsync() X_FINAL;

	bool valid(void) const;
	bool supportsComplitionRoutine(void) const;

	Type::Enum getType(void) const X_FINAL;

	XFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position) X_FINAL;
	XFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position) X_FINAL;

	XFileAsyncOperationCompiltion readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack);

	void cancelAll(void) const X_FINAL;

	// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
	size_t waitUntilFinished(const XFileAsyncOperation& operation) X_FINAL;

	uint64_t fileSize(void) const X_FINAL;

	void setSize(int64_t numBytes) X_FINAL;

#if X_ENABLE_FILE_STATS
	static XFileStats& fileStats(void);
#endif // !X_ENABLE_FILE_STATS

private:
	void Job_copyData(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* jobData);
	void Job_InflateData(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* job, void* jobData);

private:
	uint64_t getPosition(uint64_t pos) const;
	uint64_t getLength(size_t length, uint64_t pos) const;

private:
	core::MemoryArenaBase* overlappedArena_;
	Pak* pPak_;
	const AssetPak::APakEntry& entry_;
};

X_INLINE XPakFileAsync::Type::Enum XPakFileAsync::getType(void) const
{
	return Type::VIRTUAL;
}


X_NAMESPACE_END
