#include "stdafx.h"
#include "WeaponManager.h"

#include <Assets\AssetLoader.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{

	WeaponDefManager::WeaponDefManager(core::MemoryArenaBase* arena) :
		arena_(arena),
		pAssetLoader_(nullptr),
		pDefaultWeaponDef_(nullptr),
		weaponDefs_(arena, sizeof(WeaponDefResource), X_ALIGN_OF(WeaponDefResource), "WeaponDefs")
	{

	}

	void WeaponDefManager::registerCmds(void)
	{

	}

	void WeaponDefManager::registerVars(void)
	{

	}

	bool WeaponDefManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);

		pAssetLoader_ = gEnv->pCore->GetAssetLoader();
		pAssetLoader_->registerAssetType(assetDb::AssetType::WEAPON, this, WEAPON_FILE_EXTENSION);

		if (!initDefaults()) {
			return false;
		}

		gEnv->pHotReload->addfileType(this, WEAPON_FILE_EXTENSION);

		return true;
	}

	void WeaponDefManager::shutDown(void)
	{
		gEnv->pHotReload->unregisterListener(this);


		if (pDefaultWeaponDef_) {
			releaseWeaponDef(pDefaultWeaponDef_);
		}

		freeDangling();
	}

	bool WeaponDefManager::asyncInitFinalize(void)
	{
		if (!pDefaultWeaponDef_) {
			X_ERROR("WeaponDef", "Default WeaponDef is not valid");
			return false;
		}

		if (!waitForLoad(pDefaultWeaponDef_)) {
			X_ERROR("WeaponDef", "Failed to load default WeaponDef");
			return false;
		}

		return true;
	}

	WeaponDef* WeaponDefManager::findWeaponDef(const char* pName) const
	{
		core::string name(pName);

		core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

		WeaponDef* pDef = weaponDefs_.findAsset(name);
		if (pDef) {
			return pDef;
		}

		X_WARNING("WeaponDef", "Failed to find material: \"%s\"", pName);
		return nullptr;
	}

	WeaponDef* WeaponDefManager::loadWeaponDef(const char* pName)
	{
		X_ASSERT(core::strUtil::FileExtension(pName) == nullptr, "Extension not allowed")(pName);

		core::string name(pName);
		WeaponDefResource* pWeaponDefRes = nullptr;

		core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

		{
			pWeaponDefRes = weaponDefs_.findAsset(name);
			if (pWeaponDefRes)
			{
				// inc ref count.
				pWeaponDefRes->addReference();
				return pWeaponDefRes;
			}

			pWeaponDefRes = weaponDefs_.createAsset(name, name);
		}

		addLoadRequest(pWeaponDefRes);

		return pWeaponDefRes;
	}

	WeaponDef* WeaponDefManager::getDefaultDef(void) const
	{
		X_ASSERT_NOT_IMPLEMENTED();
		return nullptr;
	}



	bool WeaponDefManager::waitForLoad(core::AssetBase* pWeaponDef)
	{
		X_ASSERT(pWeaponDef->getType() == assetDb::AssetType::WEAPON, "Invalid asset passed")();

		if (pWeaponDef->isLoaded()) {
			return true;
		}

		return waitForLoad(static_cast<WeaponDef*>(pWeaponDef));
	}


	bool WeaponDefManager::waitForLoad(WeaponDef* pWeaponDef)
	{
		if (pWeaponDef->getStatus() == core::LoadStatus::Complete) {
			return true;
		}

		return pAssetLoader_->waitForLoad(pWeaponDef);
	}

	void WeaponDefManager::releaseWeaponDef(WeaponDef* pWeaponDef)
	{
		WeaponDefResource* pWeaponDefRes = reinterpret_cast<WeaponDefResource*>(pWeaponDef);
		if (pWeaponDefRes->removeReference() == 0)
		{
			weaponDefs_.releaseAsset(pWeaponDefRes);
		}
	}

	bool WeaponDefManager::initDefaults(void)
	{
		if (!pDefaultWeaponDef_)
		{
			pDefaultWeaponDef_ = loadWeaponDef(WEAPON_DEFAULT_NAME);
			if (!pDefaultWeaponDef_) {
				X_ERROR("WeaponDef", "Failed to create default weapondef");
				return false;
			}
		}

		return true;
	}

	void WeaponDefManager::freeDangling(void)
	{

	}

	void WeaponDefManager::addLoadRequest(WeaponDefResource* pWeaponDef)
	{
		pAssetLoader_->addLoadRequest(pWeaponDef);
	}

	void WeaponDefManager::onLoadRequestFail(core::AssetBase* pAsset)
	{
		auto* pWeaponDef = static_cast<WeaponDef*>(pAsset);

		if (pWeaponDef != pDefaultWeaponDef_)
		{
			if (!pDefaultWeaponDef_->isLoaded())
			{
				X_ASSERT_NOT_IMPLEMENTED();
			}
			else
			{
				// need to copy a goat, mind the horns.
				pWeaponDef->assignProps(*pDefaultWeaponDef_);
			}
		}
	}

	bool WeaponDefManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
	{
		auto* pWeaponDef = static_cast<WeaponDef*>(pAsset);

		core::XFileFixedBuf file(data.ptr(), data.ptr() + dataSize);
		
		return pWeaponDef->processData(&file);
	}

	void WeaponDefManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys, name);


	}


} // namespace weapon

X_NAMESPACE_END