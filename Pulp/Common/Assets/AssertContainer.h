#pragma once

#ifndef X_BASE_ASSET_H_
#define X_BASE_ASSET_H_

#include <Threading\AtomicInt.h>
#include <Threading\ScopedLock.h>
#include <Containers\HashMap.h>
#include <Containers\Array.h>
#include <Containers\Fifo.h>

#include <String\Path.h>

// for new
#include <IAssetDb.h>
#include <Util\BitUtil.h>
#include <Util\ReferenceCounted.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\HeapArea.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(core)

template<typename AssetType, size_t MaxAssets, class MemoryThreadPolicy>
class AssetPool
{
public:
    typedef core::MemoryArena<
        core::PoolAllocator,
        MemoryThreadPolicy,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        AssetPoolArena;

public:
    AssetPool(core::MemoryArenaBase* arena, size_t allocSize, size_t allocAlign, const char* pArenaName) :
        assetPoolHeap_(
            core::bitUtil::RoundUpToMultiple<size_t>(
                AssetPoolArena::getMemoryRequirement(allocSize) * MaxAssets,
                core::VirtualMem::GetPageSize())),
        assetPoolAllocator_(assetPoolHeap_.start(), assetPoolHeap_.end(),
            AssetPoolArena::getMemoryRequirement(allocSize),
            AssetPoolArena::getMemoryAlignmentRequirement(allocAlign),
            AssetPoolArena::getMemoryOffsetRequirement()),
        assetPoolArena_(&assetPoolAllocator_, pArenaName)
    {
        arena->addChildArena(&assetPoolArena_);
    }

    template<class... Args>
    X_INLINE AssetType* allocate(Args&&... args)
    {
        return X_NEW(AssetType, &assetPoolArena_, "AsetRes")(std::forward<Args>(args)...);
    }

    X_INLINE void free(AssetType* pRes)
    {
        X_DELETE(pRes, &assetPoolArena_);
    }

private:
    core::HeapArea assetPoolHeap_;
    core::PoolAllocator assetPoolAllocator_;
    AssetPoolArena assetPoolArena_;
};

template<typename AssetType, size_t MaxAssets, class MemoryThreadPolicy,
    typename ResourceRefPrim = typename std::conditional<std::is_same<MemoryThreadPolicy, core::SingleThreadPolicy>::value, int32_t, core::AtomicInt>::type>
class AssetPoolRefCounted
{
public:
    typedef ResourceRefPrim RefPrim;
    typedef core::ReferenceCountedInherit<AssetType, RefPrim> AssetResource;

    typedef core::MemoryArena<
        core::PoolAllocator,
        MemoryThreadPolicy,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        AssetPoolArena;

public:
    AssetPoolRefCounted(core::MemoryArenaBase* arena, size_t allocSize, size_t allocAlign, const char* pArenaName) :
        assetPoolHeap_(
            core::bitUtil::RoundUpToMultiple<size_t>(
                AssetPoolArena::getMemoryRequirement(allocSize) * MaxAssets,
                core::VirtualMem::GetPageSize())),
        assetPoolAllocator_(assetPoolHeap_.start(), assetPoolHeap_.end(),
            AssetPoolArena::getMemoryRequirement(allocSize),
            AssetPoolArena::getMemoryAlignmentRequirement(allocAlign),
            AssetPoolArena::getMemoryOffsetRequirement()),
        assetPoolArena_(&assetPoolAllocator_, pArenaName)
    {
        arena->addChildArena(&assetPoolArena_);
    }

    template<class... Args>
    X_INLINE AssetResource* allocate(Args&&... args)
    {
        return X_NEW(AssetResource, &assetPoolArena_, "AsetRes")(std::forward<Args>(args)...);
    }

    X_INLINE void free(AssetResource* pRes)
    {
        X_DELETE(pRes, &assetPoolArena_);
    }

private:
    core::HeapArea assetPoolHeap_;
    core::PoolAllocator assetPoolAllocator_;
    AssetPoolArena assetPoolArena_;
};

template<typename AssetType, size_t MaxAssets, class ThreadPolicy>
class AssetContainer : private AssetPoolRefCounted<AssetType, MaxAssets, core::SingleThreadPolicy, core::AtomicInt>
{
    typedef AssetPoolRefCounted<
        AssetType,
        MaxAssets,
        core::SingleThreadPolicy,
        core::AtomicInt // i've made this atmoic so that asset refs can be modified without having to take a lock on container.
        >
        Pool;

public:
    typedef typename Pool::AssetResource Resource;
    typedef core::HashMap<core::string, Resource*> ResourceMap;
    typedef core::Array<Resource*> ResourceList;
    typedef core::Fifo<int32_t> IndexList;

    typedef typename ResourceMap::iterator ResourceItor;
    typedef typename ResourceMap::const_iterator ResourceConstItor;
    typedef ThreadPolicy ThreadPolicy;

public:
    AssetContainer(core::MemoryArenaBase* arena, size_t allocSize, size_t allocAlign, const char* pArenaName) :
        hash_(arena, MaxAssets),
        list_(arena),
        freeList_(arena),
        Pool(arena, allocSize, allocAlign, pArenaName)
    {
        list_.reserve(MaxAssets);
    }

    X_INLINE ThreadPolicy& getThreadPolicy(void) const
    {
        return threadPolicy_;
    }

    X_INLINE void free(void)
    {
        core::ScopedLock<ThreadPolicy> lock(threadPolicy_);

        for (const auto& it : hash_) {
            Pool::free(it.second);
        }

        hash_.free();
        list_.free();
        freeList_.free();
    }

    X_INLINE Resource* findAsset(const core::string& name) const
    {
        X_ASSERT(name.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr,"asset name has invalid slash")(name.c_str());

        auto it = hash_.find(name);
        if (it != hash_.end()) {
            return it->second;
        }

        return nullptr;
    }

    X_INLINE Resource* findAsset(int32_t id) const
    {
        return list_[id];
    }

    template<class... Args>
    Resource* createAsset(const core::string& name, Args&&... args)
    {
        X_ASSERT(name.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr,"asset name has invalid slash")(name.c_str());

        Resource* pRes = Pool::allocate(std::forward<Args>(args)...);

        hash_.insert(typename ResourceMap::value_type(name, pRes));

        int32_t id;

        if (freeList_.isNotEmpty()) {
            id = freeList_.peek();
            freeList_.pop();

            list_[id] = pRes;
        }
        else {
            id = safe_static_cast<int32_t>(list_.size());
            list_.append(pRes);
        }

        pRes->setID(id);
        return pRes;
    }

    X_INLINE void releaseAsset(Resource* pRes)
    {
        core::ScopedLock<ThreadPolicy> lock(threadPolicy_);

        X_ASSERT(pRes->getRefCount() == 0, "Tried to release asset with refs")(pRes->getRefCount());

        auto numErase = hash_.erase(pRes->getName());

        // we should earse only one asset
        X_ASSERT(numErase == 1, "Failed to erase asset correct")();

        // get id before free.
        const auto id = pRes->getID();

        X_ASSERT(id >= 0 && id < static_cast<decltype(id)>(list_.capacity()), "Id out of range")(id, list_.capacity());

#if X_DEBUG
        list_[id] = nullptr;
#endif // |X_DEBUG

        Pool::free(pRes);

        freeList_.push(id);
    }

public:
    X_INLINE ResourceItor begin(void)
    {
        return hash_.begin();
    }
    X_INLINE ResourceConstItor begin(void) const
    {
        return hash_.begin();
    }
    X_INLINE ResourceItor end(void)
    {
        return hash_.end();
    }
    X_INLINE ResourceConstItor end(void) const
    {
        return hash_.end();
    }

    X_INLINE const size_t size(void) const
    {
        return hash_.size();
    }

private:
    mutable ThreadPolicy threadPolicy_;

    ResourceMap hash_;
    ResourceList list_;
    IndexList freeList_;
};

X_NAMESPACE_END

#endif // !X_BASE_ASSET_H_