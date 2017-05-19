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

class XResourceContainer;

class XBaseAsset
{
	friend class XResourceContainer;

public:
	XBaseAsset() : ID_(0xFFFFFFFF), RefCount_(1), pContainer_(nullptr) {}
	XBaseAsset(const XBaseAsset& Src);
	XBaseAsset& operator=(const XBaseAsset& Src);

	virtual ~XBaseAsset() {};

	X_INLINE int getID() const { return ID_; }
	X_INLINE void setID(int nID) { ID_ = nID; }
	X_INLINE const char* resourceName(void) const { return resourceName_.c_str(); }

	virtual bool isValid() const {
		return false;
	}

	virtual const int addRef() {
		++RefCount_;
		return RefCount_;
	}
	virtual const int release();
	virtual const int getRefCounter() const { return RefCount_; }


protected:
	uint32_t ID_;
	core::AtomicInt RefCount_; // thread safe.
	core::string resourceName_;
	XResourceContainer* pContainer_;
};



class XResourceContainer
{
public:
	friend class XBaseAsset;

public:
	XResourceContainer(core::MemoryArenaBase* arena, size_t size) :
		list(arena),
		hash(arena)
	{
		if (arena) {
			list.reserve(size);
			hash.reserve(size);
		}
	}

	void setArena(core::MemoryArenaBase* arena, size_t size) {
		X_ASSERT_NOT_NULL(arena);

		list.setArena(arena);
		hash.setArena(arena);

		list.reserve(size);
		hash.reserve(size);
	}

	void free(void) {
		list.free();
		hash.free();
	}

	XBaseAsset* findAsset(const char* name) const {
		X_ASSERT_NOT_NULL(name);
		X_ASSERT(core::strUtil::Find(name, name + strlen(name), '\\') == nullptr,
			"asset name must have forward slash")(name);

		ResourceConstItor it = hash.find(X_CONST_STRING(name));
		if (it != hash.end())
			return it->second;
		return nullptr; // O'Deer
	}

	// fast id maps to index.
	XBaseAsset* findAsset(uint32_t id) {
		if (id < 0 || id > list.size()) // needed for release?
			return nullptr; // O'Deer
		return list[id];
	}

	void AddAsset(const char* name, XBaseAsset* pAsset) {
		X_ASSERT_NOT_NULL(name);
		X_ASSERT_NOT_NULL(pAsset);
		X_ASSERT(core::strUtil::Find(name, name + strlen(name), '\\') == nullptr,
			"asset name must have forward slash")(name);

		hash.insert(ResourceItor::value_type(core::string(name), pAsset));

		uint32_t id = safe_static_cast<uint32_t, size_t>(list.size());
		pAsset->setID(id);
		pAsset->resourceName_ = name;
		pAsset->pContainer_ = this;

		// the id is the index of the asset 
		// allowing fast retrival of the asset by id.
		list.append(pAsset);
	}

protected:

	virtual bool removeAsset(XBaseAsset* pAsset);

private:
	typedef core::Array<XBaseAsset*> ResourceList;
	typedef core::HashMap<core::string, XBaseAsset*> ResourceMap;

	ResourceList list;
	ResourceMap hash;

public:
	typedef ResourceMap::iterator ResourceItor;
	typedef ResourceMap::const_iterator ResourceConstItor;

	ResourceItor begin(void) {
		return hash.begin();
	}
	ResourceConstItor begin(void) const {
		return hash.begin();
	}

	ResourceItor end(void) {
		return hash.end();
	}
	ResourceConstItor end(void) const {
		return hash.end();
	}

	const size_t size(void) const {
		return list.size();
	}

};

// ---------------------------------------------------------------------------


template<typename AssetType, size_t MaxAssets, class MemoryThreadPolicy, 
	typename ResourceRefPrim = std::conditional<std::is_same<MemoryThreadPolicy, core::SingleThreadPolicy>::value, int32_t, core::AtomicInt>::type>
class AssetPool
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
	> AssetPoolArena;

public:
	AssetPool(core::MemoryArenaBase* arena, size_t allocSize, size_t allocAlign) :
		assetPoolHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				AssetPoolArena::getMemoryRequirement(allocSize) * MaxAssets,
				core::VirtualMem::GetPageSize()
				)
		),
		assetPoolAllocator_(assetPoolHeap_.start(), assetPoolHeap_.end(),
			AssetPoolArena::getMemoryRequirement(allocSize),
			AssetPoolArena::getMemoryAlignmentRequirement(allocAlign),
			AssetPoolArena::getMemoryOffsetRequirement()
		),
		assetPoolArena_(&assetPoolAllocator_, "AssetPoolArena")
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
	core::HeapArea      assetPoolHeap_;
	core::PoolAllocator assetPoolAllocator_;
	AssetPoolArena		assetPoolArena_;
};

template<typename AssetType, size_t MaxAssets, class ThreadPolicy>
class AssetContainer : private AssetPool<AssetType, MaxAssets, core::SingleThreadPolicy, core::AtomicInt>
{
	typedef AssetPool<AssetType, 
		MaxAssets, 
		core::SingleThreadPolicy,
		core::AtomicInt				// i've made this atmoic so that asset refs can be modified without having to take a lock on container.
	> Pool;
public:
	typedef Pool::AssetResource Resource;
	typedef core::HashMap<core::string, Resource*> ResourceMap;
	typedef core::Array<Resource*> ResourceList;
	typedef core::Fifo<int32_t> IndexList;

	typedef typename ResourceMap::iterator ResourceItor;
	typedef typename ResourceMap::const_iterator ResourceConstItor;
	typedef typename ThreadPolicy ThreadPolicy;

public:
	AssetContainer(core::MemoryArenaBase* arena, size_t allocSize, size_t allocAlign) :
		hash_(arena, MaxAssets),
		list_(arena),
		freeList_(arena),
		AssetPool(arena, allocSize, allocAlign)
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
	}

	X_INLINE Resource* findAsset(const core::string& name) const
	{
		X_ASSERT(name.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr,
			"asset name has invalid slash")(name.c_str());
	
		auto it = hash_.find(name);
		if (it != hash_.end()) {
			return it->second;
		}

		return nullptr;
	}

	X_INLINE Resource* findAsset(uint32_t id) const
	{
		return list_[id];
	}

	template<class... Args>
	Resource* createAsset(const core::string& name, Args&&... args)
	{
		X_ASSERT(name.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr,
			"asset name has invalid slash")(name.c_str());

		Resource* pRes = Pool::allocate(std::forward<Args>(args)...);

		hash_.insert(ResourceMap::value_type(name, pRes));

		int32_t id;

		if (freeList_.isNotEmpty())
		{
			id = freeList_.peek();
			freeList_.pop();

			list_[id] = pRes;
		}
		else
		{
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

	X_INLINE ResourceItor begin(void) {
		return hash_.begin();
	}
	X_INLINE ResourceConstItor begin(void) const {
		return hash_.begin();
	}
	X_INLINE ResourceItor end(void) {
		return hash_.end();
	}
	X_INLINE ResourceConstItor end(void) const {
		return hash_.end();
	}

	X_INLINE const size_t size(void) const {
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