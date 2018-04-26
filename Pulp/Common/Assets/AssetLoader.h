#pragma once

#include <IAssetDb.h>

#include <Util\UniquePointer.h>
#include <Containers\Array.h>
#include <Containers\Fifo.h>

X_NAMESPACE_BEGIN(core)

namespace V2
{
    struct Job;
    class JobSystem;
} // namespace V2
struct XFileAsync;
struct IoRequestBase;

struct IAssetLoadSink;
class AssetBase;

X_DECLARE_FLAGS8(LoadFlag)
(
    Reload);

typedef Flags8<LoadFlag> LoadFlags;

X_DECLARE_FLAGS8(ReloadFlag)
(
    AnyTime,
    Beginframe);

typedef Flags8<ReloadFlag> ReloadFlags;


class AssetLoaderVars
{
public:
    AssetLoaderVars();
    ~AssetLoaderVars() = default;

    void registerVars(void);


    bool debugEnabled(void) const;

private:
    int32_t debug_;
};


class AssetLoader
{
    typedef std::array<IAssetLoadSink*, assetDb::AssetType::ENUM_COUNT> AssetLoadSinksArr;
    typedef std::array<const char*, assetDb::AssetType::ENUM_COUNT> AssetExtArr;

    struct AssetLoadRequest
    {
        X_INLINE AssetLoadRequest(AssetBase* pAsset) :
            pFile(nullptr),
            pAsset(pAsset),
            dataSize(0)
        {
        }

        core::XFileAsync* pFile;
        AssetBase* pAsset;
        core::UniquePointer<char[]> data;
        uint32_t dataSize;
        LoadFlags flags;
        ReloadFlags reloadFlags;
        uint16_t _pad;
    };

    typedef core::Array<AssetLoadRequest*> AssetLoadRequestArr;
    typedef core::Fifo<AssetBase*> AssetQueue;

public:
    AssetLoader(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);

    void registerVars(void);

    void registerAssetType(assetDb::AssetType::Enum type, IAssetLoadSink* pSink, const char* pExt);

    void update(void);

    void reload(AssetBase* pAsset, ReloadFlags flags);
    void addLoadRequest(AssetBase* pAsset);
    bool waitForLoad(AssetBase* pAsset);

    void dispatchPendingLoads(void);

private:
    void queueLoadRequest(AssetBase* pAsset, core::CriticalSection::ScopedLock&);
    void dispatchLoad(AssetBase* pAsset, core::CriticalSection::ScopedLock&);
    bool dispatchPendingLoad(core::CriticalSection::ScopedLock&);
    void dispatchLoadRequest(AssetLoadRequest* pLoadReq);

    // load / processing
    void onLoadRequestFail(AssetLoadRequest* pLoadReq);
    void loadRequestCleanup(AssetLoadRequest* pLoadReq);

private:
    void IoRequestCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t);

    void processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void processData(AssetLoadRequest* pRequest);

private:
    core::MemoryArenaBase* arena_;
    core::MemoryArenaBase* blockArena_;

    AssetLoaderVars vars_;

    core::CriticalSection loadReqLock_;
    core::ConditionVariable loadCond_;

    AssetQueue requestQueue_;             // requests not yet currenty dispatched
    AssetLoadRequestArr pendingRequests_; // active requests.
    AssetLoadRequestArr pendingReloads_;

    AssetLoadSinksArr assetsinks_;
    AssetExtArr assetExt_;
};

X_NAMESPACE_END