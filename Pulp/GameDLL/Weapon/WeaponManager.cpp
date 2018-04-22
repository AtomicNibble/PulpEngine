#include "stdafx.h"
#include "WeaponManager.h"

#include <Assets\AssetLoader.h>
#include <String\AssetName.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    WeaponDefManager::WeaponDefManager(core::MemoryArenaBase* arena) :
        arena_(arena),
        pAssetLoader_(nullptr),
        pDefaultWeaponDef_(nullptr),
        weaponDefs_(arena, sizeof(WeaponDefResource), X_ALIGN_OF(WeaponDefResource), "WeaponDefs"),
        ammoTypeList_(arena)
    {
    }

    void WeaponDefManager::registerCmds(void)
    {
        ADD_COMMAND_MEMBER("listWeapons", this, WeaponDefManager, &WeaponDefManager::Cmd_List,
            core::VarFlag::SYSTEM, "List all the loaded Weapons");

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
            if (pWeaponDefRes) {
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
        if (pWeaponDefRes->removeReference() == 0) {
            weaponDefs_.releaseAsset(pWeaponDefRes);
        }
    }

    AmmoTypeId WeaponDefManager::getAmmoTypeId(const char* pName)
    {
        core::StrHash hash(pName);

        core::CriticalSection::ScopedLock lock(cs_);

        for (size_t i = 0; i < ammoTypeList_.size(); i++) {
            if (ammoTypeList_[i] == hash) {
                return safe_static_cast<AmmoTypeId>(i);
            }
        }

        if (ammoTypeList_.size() < weapon::WEAPON_MAX_AMMO_TYPES) {
            return safe_static_cast<AmmoTypeId>(ammoTypeList_.push_back(hash));
        }

        // tut tut.
        X_ERROR("WeaponDef", "Exceeded max(%" PRIu32 ") AmmoTypes", weapon::WEAPON_MAX_AMMO_TYPES);
        return INVALID_AMMO_TYPE;
    }

    bool WeaponDefManager::initDefaults(void)
    {
        if (!pDefaultWeaponDef_) {
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

        if (pWeaponDef != pDefaultWeaponDef_) {
            if (!pDefaultWeaponDef_->isLoaded()) {
                X_ASSERT_NOT_IMPLEMENTED();
            }
            else {
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

        core::AssetName assetName(name);
        assetName.replaceSeprators();
        assetName.stripAssetFolder(assetDb::AssetType::WEAPON);
        assetName.removeExtension();

        core::string nameStr(assetName.begin(), assetName.end());

        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        auto* pWeaponRes = weaponDefs_.findAsset(nameStr);
        if (!pWeaponRes) {
            X_LOG1("WeaponDef", "Not reloading \"%s\" it's not currently used", nameStr.c_str());
            return;
        }

        X_LOG0("WeaponDef", "Reloading: %s", nameStr.c_str());

        pAssetLoader_->reload(pWeaponRes, core::ReloadFlag::Beginframe);
    }


    void WeaponDefManager::listWeapons(const char* pSearchPatten) const
    {
        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        core::Array<WeaponDefResource*> sorted(arena_);
        sorted.setGranularity(weaponDefs_.size());

        for (const auto& wpn : weaponDefs_) {
            auto* pWpnRes = wpn.second;

            if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pWpnRes->getName())) {
                sorted.push_back(pWpnRes);
            }
        }

        std::sort(sorted.begin(), sorted.end(), [](WeaponDefResource* a, WeaponDefResource* b) {
            const auto& nameA = a->getName();
            const auto& nameB = b->getName();
            return nameA.compareInt(nameB) < 0;
        });

        X_LOG0("WeaponDef", "------------ ^8Weapons(%" PRIuS ")^7 ---------------", sorted.size());

        for (const auto* pWpn : sorted) {
            X_LOG0("WeaponDef", "^2%-32s^7 Refs: ^2%i",
                pWpn->getName(), pWpn->getRefCount());
        }

        X_LOG0("WeaponDef", "------------ ^8Weapnos End^7 --------------");
    }


    void WeaponDefManager::Cmd_List(core::IConsoleCmdArgs* pCmd)
    {
        const char* pSearchPatten = nullptr;

        if (pCmd->GetArgCount() >= 2) {
            pSearchPatten = pCmd->GetArg(1);
        }

        listWeapons(pSearchPatten);
    }

} // namespace weapon

X_NAMESPACE_END