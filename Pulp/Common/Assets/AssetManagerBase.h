#pragma once


#include <Assets\AssetBase.h>


X_NAMESPACE_BEGIN(core)

namespace V2 {
	struct Job;
	class JobSystem;
}
struct XFileAsync;
struct IoRequestBase;


class AssetManagerBase
{
protected:
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
	};


	typedef core::Array<AssetLoadRequest*> LoadRequestArr;

public:

	typedef AssetBase Asset;
	typedef AssetBase* AssetPtr;

public:
	AssetManagerBase(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena);

protected:

	// TODO:

protected:
	core::MemoryArenaBase*		arena_;
	core::MemoryArenaBase*		blockArena_;

	core::CriticalSection		loadReqLock_;
	core::ConditionVariable		loadCond_;

	LoadRequestArr				pendingRequests_;
};

X_INLINE AssetManagerBase::AssetManagerBase(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
	arena_(arena),
	blockArena_(blockArena),
	pendingRequests_(arena)
{

}


X_NAMESPACE_END