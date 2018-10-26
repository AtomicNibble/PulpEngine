#include "stdafx.h"
#include "PakFile.h"

#include "xFileSys.h"
#include "Pak.h"

#include <ICompression.h>
#include <Compression\CompressorAlloc.h>

#include X_INCLUDE(X_PLATFORM/OsFileAsync.h)

X_NAMESPACE_BEGIN(core)

namespace
{
#if X_ENABLE_FILE_STATS
    static XFileStats s_stats;
#endif // !X_ENABLE_FILE_STATS

} // namespace

XPakFile::XPakFile(Pak* pPack, const AssetPak::APakEntry& entry) :
    pPak_(pPack),
    entry_(entry),
    offset_(0)
{
    ++pPak_->openHandles;
}

XPakFile::~XPakFile()
{
    --pPak_->openHandles;
}

size_t XPakFile::read(void* pBuffer, size_t length)
{
#if X_ENABLE_FILE_STATS
    s_stats.NumBytesRead += length;
    ++s_stats.NumReads;
#endif // !X_ENABLE_FILE_STATS

    if (entry_.isCompressed()) {
        X_ASSERT_NOT_IMPLEMENTED();
    }

    uint64_t pakPos = getPosition();
    size_t pakLength = getLength(length);

    if (pPak_->mode == PakMode::MEMORY) 
    {
        auto* pSrc = &pPak_->data[safe_static_cast<size_t>(pakPos)];

        std::memcpy(pBuffer, pSrc, pakLength);
    }
    else
    {
        auto op = pPak_->pFile->readAsync(pBuffer, pakLength, pakPos);
        auto bytesRead = op.waitUntilFinished();

        pakLength = bytesRead;
    }

    return pakLength;
}

size_t XPakFile::write(const void* pBuffer, size_t length)
{
    // not allowed to write to pak
    X_UNUSED(pBuffer, length);
    X_ASSERT_UNREACHABLE();
    return 0;
}

void XPakFile::seek(int64_t position, SeekMode::Enum origin)
{
    X_UNUSED(position, origin);
    X_ASSERT_NOT_IMPLEMENTED();
}

uint64_t XPakFile::fileSize(void) const
{
    return entry_.inflatedSize;
}

uint64_t XPakFile::remainingBytes(void) const
{
    return fileSize() - offset_;
}

uint64_t XPakFile::tell(void) const
{
    return offset_;
}

void XPakFile::setSize(int64_t numBytes)
{
    X_UNUSED(numBytes);
    X_ASSERT_NOT_IMPLEMENTED();
}

bool XPakFile::valid(void) const
{
    return true;
}

X_INLINE uint64_t XPakFile::getPosition(void) const
{
    return pPak_->dataOffset + entry_.offset + offset_;
}

X_INLINE size_t XPakFile::getLength(size_t length) const
{
    const auto left = remainingBytes();

    if (length > left) {
        X_WARNING("PakFile", "Attempted to read past end of file in pak");
        return safe_static_cast<size_t>(left);
    }

    return length;
}

#if X_ENABLE_FILE_STATS
XFileStats& XPakFile::fileStats(void)
{
    return s_stats;
}
#endif // !X_ENABLE_FILE_STATS

X_NAMESPACE_END
