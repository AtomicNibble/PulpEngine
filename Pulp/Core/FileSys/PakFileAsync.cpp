#include "stdafx.h"
#include "PakFileAsync.h"

#include "xFileSys.h"
#include "Pak.h"

#include <Threading\JobSystem2.h>

#include <ICompression.h>
#include <Compression\CompressorAlloc.h>

#include X_INCLUDE(X_PLATFORM/OsFileAsync.h)

X_NAMESPACE_BEGIN(core)

namespace
{
#if X_ENABLE_FILE_STATS
    static XFileStats s_stats;
#endif // !X_ENABLE_FILE_STATS

    struct CopyJobData
    {
        void* pBuffer;
        size_t position; // 32bit since can't have large pak's in mmeory.
        size_t length;
    };

    struct InflateJobData
    {
        core::MemoryArenaBase* arena;
        const void* pBuffer;
        void* pDst;
        size_t length;
        size_t dstLength;
    };

} // namespace

XPakFileAsync::XPakFileAsync(Pak* pPack, const AssetPak::APakEntry& entry, core::MemoryArenaBase* asyncOpArena) :
    pPak_(pPack),
    entry_(entry),
    overlappedArena_(asyncOpArena)
{
    ++pPak_->openHandles;
}

XPakFileAsync::~XPakFileAsync()
{
    --pPak_->openHandles;
}

void XPakFileAsync::Job_copyData(core::V2::JobSystem&, size_t, core::V2::Job*, void* jobData)
{
    const CopyJobData* pData = static_cast<const CopyJobData*>(jobData);

    std::memcpy(pData->pBuffer, &pPak_->data[pData->position], pData->length);
}

void XPakFileAsync::Job_InflateData(core::V2::JobSystem&, size_t, core::V2::Job*, void* jobData)
{
    const InflateJobData* pData = static_cast<const InflateJobData*>(jobData);
    const uint8_t* pBuffer = static_cast<const uint8_t*>(pData->pBuffer);
    uint8_t* pDst = static_cast<uint8_t*>(pData->pDst);

    if (!core::Compression::ICompressor::validBuffer(make_span(pBuffer, pData->length))) {
        X_FATAL("PakFile", "Data is not a valid compressed buffer");
        return;
    }

    auto algo = core::Compression::ICompressor::getAlgo(make_span(pBuffer, pData->length));
    core::Compression::CompressorAlloc comp(algo);

    if (!comp->inflate(pData->arena, pBuffer, pBuffer + pData->length, pDst, pDst + pData->dstLength)) {
        // shiieeet.
        X_FATAL("PakFile", "failed to inflate");
    }
}

XFileAsyncOperation XPakFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position)
{
    if (entry_.isCompressed()) {
        X_ASSERT(length == fileSize() && position == 0, "Partial reads not supported for compressed files.")(length, position, fileSize());
    }

    uint64_t pakPos = getPosition(position);
    size_t pakLength = getLength(length, position);

    if (pPak_->mode == PakMode::MEMORY) {
#if X_ENABLE_FILE_STATS
        s_stats.NumBytesRead += pakLength;
        ++s_stats.NumReads;
#endif // !X_ENABLE_FILE_STATS

        auto* pSrc = &pPak_->data[safe_static_cast<size_t>(pakPos)];
        auto length32 = safe_static_cast<uint32_t>(pakLength);

        if (entry_.isCompressed()) {
            InflateJobData data;
            data.arena = g_coreArena;
            data.pBuffer = pSrc;
            data.length = entry_.size;
            data.pDst = pBuffer;
            data.dstLength = pakLength;

            auto* pJob = gEnv->pJobSys->CreateMemberJobAndRun<XPakFileAsync>(this, &XPakFileAsync::Job_InflateData, data JOB_SYS_SUB_PASS(profiler::SubSys::CORE));

            return XFileAsyncOperation(overlappedArena_, length32, pJob);
        }

        if (length32 > (1024 * 1024 * 4)) {
            CopyJobData data;
            data.pBuffer = pBuffer;
            data.length = pakLength;
            data.position = safe_static_cast<size_t>(pakPos);

            auto* pJob = gEnv->pJobSys->CreateMemberJobAndRun<XPakFileAsync>(this, &XPakFileAsync::Job_copyData, data JOB_SYS_SUB_PASS(profiler::SubSys::CORE));

            return XFileAsyncOperation(overlappedArena_, length32, pJob);
        }

        std::memcpy(pBuffer, pSrc, pakLength);

        return XFileAsyncOperation(overlappedArena_, length32);
    }

    return pPak_->pFile->readAsync(pBuffer, pakLength, pakPos);
}

XFileAsyncOperation XPakFileAsync::writeAsync(const void* pBuffer, size_t length, uint64_t position)
{
    // not allowed to write to pak
    X_ASSERT_UNREACHABLE();

    uint64_t pakPos = getPosition(position);
    size_t pakLength = getLength(length, position);

    return pPak_->pFile->writeAsync(pBuffer, pakLength, pakPos);
}

XFileAsyncOperationCompiltion XPakFileAsync::readAsync(void* pBuffer, size_t length, uint64_t position, ComplitionRotinue callBack)
{
    X_ASSERT(supportsComplitionRoutine(), "Competition routine not supported")(supportsComplitionRoutine());

    uint64_t pakPos = getPosition(position);
    size_t pakLength = getLength(length, position);

    return pPak_->pFile->readAsync(pBuffer, pakLength, pakPos, callBack);
}

uint64_t XPakFileAsync::fileSize(void) const
{
    if (pPak_->mode == PakMode::STREAM) {
        return entry_.size;
    }

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

bool XPakFileAsync::supportsComplitionRoutine(void) const
{
    return pPak_->mode == PakMode::STREAM && !entry_.isCompressed();
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
    return pPak_->dataOffset + entry_.offset + pos;
}

X_INLINE size_t XPakFileAsync::getLength(size_t length, uint64_t pos) const
{
    const auto end = pos + length;
    const auto size = fileSize();

    if (end > size) {
        X_WARNING("PakFile", "Attempted to read past end of file in pak");
        return safe_static_cast<size_t>(size - pos);
    }

    return length;
}

#if X_ENABLE_FILE_STATS
XFileStats& XPakFileAsync::fileStats(void)
{
    return s_stats;
}
#endif // !X_ENABLE_FILE_STATS

X_NAMESPACE_END
