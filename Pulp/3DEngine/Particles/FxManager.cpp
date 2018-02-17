#include "stdafx.h"
#include "FxManager.h"

#include <Assets\AssetLoader.h>

#include "Effect.h"
#include "Emitter.h"

X_NAMESPACE_BEGIN(engine)

namespace fx
{


	static const size_t POOL_ALLOC_MAX = EFFECT_MAX_EMITTERS;
	static const size_t POOL_ALLOCATION_SIZE = sizeof(Emitter);
	static const size_t POOL_ALLOCATION_ALIGN = X_ALIGN_OF(Emitter);


	EffectManager::EffectManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
		arena_(arena),
		blockArena_(blockArena),
		pAssetLoader_(nullptr),
		effects_(arena, sizeof(EffectResource), X_ALIGN_OF(EffectResource), "EffectPool"),
		poolHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE) * POOL_ALLOC_MAX,
				core::VirtualMem::GetPageSize()
			)
		),
		poolAllocator_(
			poolHeap_.start(), poolHeap_.end(),
			PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
			PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
			PoolArena::getMemoryOffsetRequirement()
		),
		poolArena_(&poolAllocator_, "PacketPool")
	{
		arena->addChildArena(&poolArena_);
	}

	EffectManager::~EffectManager()
	{

	}

	void EffectManager::registerCmds(void)
	{

	}

	void EffectManager::registerVars(void)
	{

	}


	bool EffectManager::init(void)
	{
		pAssetLoader_ = gEnv->pCore->GetAssetLoader();
		pAssetLoader_->registerAssetType(assetDb::AssetType::FX, this, EFFECT_FILE_EXTENSION);

		gEnv->pHotReload->addfileType(this, EFFECT_FILE_EXTENSION);

		return true;
	}

	void EffectManager::shutDown(void)
	{
		gEnv->pHotReload->unregisterListener(this);

		freeDangling();
	}


	Emitter* EffectManager::allocEmmiter(Effect* pEffect)
	{
		X_ASSERT_NOT_NULL(pEffect);

		if (waitForLoad(pEffect)) {
			return nullptr;
		}

		auto* pEmitter = X_NEW(Emitter, &poolArena_, "Emitter")(*pEffect, arena_);

		return pEmitter;
	}

	void EffectManager::freeEmmiter(Emitter* pEmitter)
	{
		X_DELETE(pEmitter, &poolArena_);
	}

	Effect* EffectManager::findEffect(const char* pAnimName) const
	{
		core::string name(pAnimName);
		core::ScopedLock<EffectContainer::ThreadPolicy> lock(effects_.getThreadPolicy());

		EffectResource* pEffectRes = effects_.findAsset(name);
		if (pEffectRes) {
			return pEffectRes;
		}

		X_WARNING("EffectManager", "Failed to find anim: \"%s\"", pAnimName);
		return nullptr;
	}


	Effect* EffectManager::loadEffect(const char* pAnimName)
	{
		X_ASSERT_NOT_NULL(pAnimName);
		X_ASSERT(core::strUtil::FileExtension(pAnimName) == nullptr, "Extension not allowed")(pAnimName);

		core::string name(pAnimName);
		core::ScopedLock<EffectContainer::ThreadPolicy> lock(effects_.getThreadPolicy());

		EffectResource* pEffectRes = effects_.findAsset(name);
		if (pEffectRes)
		{
			// inc ref count.
			pEffectRes->addReference();
			return pEffectRes;
		}

		// we create a anim and give it back
		pEffectRes = effects_.createAsset(name, name, arena_);

		// add to list of anims that need loading.
		addLoadRequest(pEffectRes);

		return pEffectRes;
	}


	void EffectManager::releaseEffect(Effect* pAnim)
	{
		auto* pEffectRes = static_cast<EffectResource*>(pAnim);
		if (pEffectRes->removeReference() == 0)
		{
			releaseResources(pEffectRes);

			effects_.releaseAsset(pEffectRes);
		}
	}

	void EffectManager::reloadEffect(const char* pName)
	{
		X_UNUSED(pName);
		X_ASSERT_NOT_IMPLEMENTED();
	}

	bool EffectManager::waitForLoad(core::AssetBase* pEffect)
	{
		X_ASSERT(pEffect->getType() == assetDb::AssetType::FX, "Invalid asset passed")();

		if (pEffect->isLoaded()) {
			return true;
		}

		return waitForLoad(static_cast<Effect*>(pEffect));
	}

	bool EffectManager::waitForLoad(Effect* pEffect)
	{
		if (pEffect->getStatus() == core::LoadStatus::Complete) {
			return true;
		}

		return pAssetLoader_->waitForLoad(pEffect);
	}


	// ------------------------------------

	void EffectManager::freeDangling(void)
	{
		{
			core::ScopedLock<EffectContainer::ThreadPolicy> lock(effects_.getThreadPolicy());

			// any left?
			for (const auto& fx : effects_)
			{
				auto* pRes = fx.second;
				const auto& name = pRes->getName();
				X_WARNING("Effect", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pRes->getRefCount());

				releaseResources(pRes);
			}
		}

		effects_.free();
	}


	void EffectManager::releaseResources(Effect* pEffect)
	{
		X_UNUSED(pEffect);


	}


	// ------------------------------------

	void EffectManager::addLoadRequest(EffectResource* pEffect)
	{
		pAssetLoader_->addLoadRequest(pEffect);
	}

	void EffectManager::onLoadRequestFail(core::AssetBase* pAsset)
	{
		auto* pEffect = static_cast<Effect*>(pAsset);

		X_UNUSED(pEffect);
	}

	bool EffectManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
	{
		auto* pEffect = static_cast<Effect*>(pAsset);

		return pEffect->processData(std::move(data), dataSize);
	}


	// -------------------------------------

	void EffectManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys, name);


	}

} // namespace fx


X_NAMESPACE_END