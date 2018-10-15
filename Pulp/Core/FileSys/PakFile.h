#pragma once

#include <IFileSys.h>
#include <IFileSysStats.h>

#include <IAssetPak.h>

X_NAMESPACE_BEGIN(core)

namespace V2
{
    class JobSystem;
    struct Job;
} // namespace V2

struct Pak;

class XPakFile : public XFile
{
public:
    XPakFile(Pak* pPack, const AssetPak::APakEntry& entry);
    ~XPakFile() X_FINAL;

    bool valid(void) const;

    size_t read(void* pBuffer, size_t length) X_FINAL;
    size_t write(const void* pBuffer, size_t length) X_FINAL;

    void seek(int64_t position, SeekMode::Enum origin) X_FINAL;

    uint64_t fileSize(void) const;
    uint64_t remainingBytes(void) const X_FINAL;
    uint64_t tell(void) const X_FINAL;

    void setSize(int64_t numBytes) X_FINAL;

#if X_ENABLE_FILE_STATS
    static XFileStats& fileStats(void);
#endif // !X_ENABLE_FILE_STATS

private:
    uint64_t getPosition(void) const;
    size_t getLength(size_t length) const;

private:
    Pak* pPak_;
    const AssetPak::APakEntry& entry_;
    int32_t offset_;
};

X_NAMESPACE_END
