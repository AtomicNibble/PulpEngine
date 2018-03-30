#include "stdafx.h"
#include "FxManager.h"

#include <Assets\AssetLoader.h>
#include <String\AssetName.h>
#include <IConsole.h>

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
		poolArena_(&poolAllocator_, "EmitterPool")
	{
		arena->addChildArena(&poolArena_);
	}

	EffectManager::~EffectManager()
	{

	}

	void EffectManager::registerCmds(void)
	{
		ADD_COMMAND_MEMBER("listEfx", this, EffectManager, &EffectManager::Cmd_ListAssets, core::VarFlag::SYSTEM,
			"List all the effects");
	}

	void EffectManager::registerVars(void)
	{
		vars_.registerVars();
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


	Emitter* EffectManager::allocEmmiter(void)
	{
		auto* pEmitter = X_NEW(Emitter, &poolArena_, "Emitter")(vars_, arena_);

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
		
		core::AssetName assetName(name);
		assetName.replaceSeprators();
		assetName.stripAssetFolder(assetDb::AssetType::FX);
		assetName.removeExtension();

		core::string nameStr(assetName.begin(), assetName.end());

		core::ScopedLock<EffectContainer::ThreadPolicy> lock(effects_.getThreadPolicy());

		EffectResource* pEffectRes = effects_.findAsset(nameStr);

		// so i wnat to reload the fx :D
		if (!pEffectRes)
		{
			X_LOG1("Effect", "Not reloading \"%s\" it's not currently used", nameStr.c_str());
			return;
		}

		X_LOG0("Effect", "Reloading: %s", nameStr.c_str());

		pAssetLoader_->reload(pEffectRes, core::ReloadFlag::AnyTime);
	}

	// -----------------------------------

	void EffectManager::listAssets(const char* pSearchPattern)
	{
		core::ScopedLock<EffectContainer::ThreadPolicy> lock(effects_.getThreadPolicy());

		core::Array<EffectContainer::Resource*> sorted_efxs(arena_);
		sorted_efxs.reserve(effects_.size());

		for (const auto& mat : effects_)
		{
			if (!pSearchPattern || core::strUtil::WildCompare(pSearchPattern, mat.first))
			{
				sorted_efxs.push_back(mat.second);
			}
		}

		std::sort(sorted_efxs.begin(), sorted_efxs.end(), [](EffectContainer::Resource* a, EffectContainer::Resource* b) {
			const auto& nameA = a->getName();
			const auto& nameB = b->getName();
			return nameA.compareInt(nameB) < 0;
		}
		);

		X_LOG0("Effect", "------------- ^8Effects(%" PRIuS ")^7 -------------", sorted_efxs.size());

		for (const auto* pEfx : sorted_efxs)
		{
			X_LOG0("Effect", "^2%-32s^7 ^7Stages: ^2%" PRIi32 " ^7Refs: ^2%" PRIi32,
				pEfx->getName(), pEfx->getNumStages(), pEfx->getRefCount());
		}

		X_LOG0("Effect", "------------ ^8Effects End^7 -------------");
	}

	// -----------------------------------

	void EffectManager::Cmd_ListAssets(core::IConsoleCmdArgs* pCmd)
	{
		const char* pSearchPattern = nullptr;

		if (pCmd->GetArgCount() >= 2) {
			pSearchPattern = pCmd->GetArg(1);
		}

		listAssets(pSearchPattern);
	}


} // namespace fx


X_NAMESPACE_END