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

        pAssetLoader_ = gEnv->pCore->GetAssetLoader();
        pAssetLoader_->registerAssetType(assetDb::AssetType::WEAPON, this, WEAPON_FILE_EXTENSION);

        if (!initDefaults()) {
            return false;
        }

        return true;
    }

    void WeaponDefManager::shutDown(void)
    {
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

    WeaponDef* WeaponDefManager::findWeaponDef(core::AssetID id) const
    {
        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        WeaponDef* pDef = weaponDefs_.findAsset(id);
        if (pDef) {
            return pDef;
        }

        return nullptr;
    }

    WeaponDef* WeaponDefManager::findWeaponDef(core::string_view name) const
    {
        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        WeaponDef* pDef = weaponDefs_.findAsset(name);
        if (pDef) {
            return pDef;
        }

        X_WARNING("WeaponDef", "Failed to find material: \"%.*s\"", name.length(), name.data());
        return nullptr;
    }

    WeaponDef* WeaponDefManager::loadWeaponDef(core::string_view name)
    {
        X_ASSERT(core::strUtil::FileExtension(name) == nullptr, "Extension not allowed")();

        WeaponDefResource* pWeaponDefRes = nullptr;

        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        {
            pWeaponDefRes = weaponDefs_.findAsset(name);
            if (pWeaponDefRes) {
                // inc ref count.
                pWeaponDefRes->addReference();
                return pWeaponDefRes;
            }

            pWeaponDefRes = weaponDefs_.createAsset(name);
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
        if (pWeaponDef->isLoaded()) {
            return true;
        }

        return pAssetLoader_->waitForLoad(pWeaponDef);
    }

    void WeaponDefManager::releaseWeaponDef(WeaponDef* pWeaponDef)
    {
        X_ASSERT(weaponDefs_.size() > 0, "Freeing a weapon def when assets is emtpy")(weaponDefs_.size(), pWeaponDef);

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
            pDefaultWeaponDef_ = loadWeaponDef(core::string_view(WEAPON_DEFAULT_NAME));
            if (!pDefaultWeaponDef_) {
                X_ERROR("WeaponDef", "Failed to create default weapondef");
                return false;
            }
        }

        return true;
    }

    // ------------------------------------

    void WeaponDefManager::freeDangling(void)
    {
        {
            core::ScopedLock<AssetContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

            for (const auto& w : weaponDefs_) {
                auto* pWpnRes = w.second;
                const auto& name = pWpnRes->getName();

                X_WARNING("WeaponDef", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pWpnRes->getRefCount());
                releaseResources(pWpnRes);
            }
        }

        weaponDefs_.free();
    }

    void WeaponDefManager::releaseResources(WeaponDef* pWeaponDef)
    {
        X_UNUSED(pWeaponDef);
    }

    // ------------------------------------


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

        if (!pWeaponDef->processData(std::move(data), dataSize)) {
            return false;
        }

        // work out the ammoType id.
        auto* pAmmoName = pWeaponDef->getStrSlot(StringSlot::AmmoName);
        auto id = getAmmoTypeId(pAmmoName);

        pWeaponDef->setAmmoTypeId(id);

        return true;
    }

    bool WeaponDefManager::onFileChanged(const core::AssetName& assetName, const core::string& name)
    {
        X_UNUSED(assetName);

        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        auto* pWeaponRes = weaponDefs_.findAsset(core::string_view(name));
        if (!pWeaponRes) {
            X_LOG1("WeaponDef", "Not reloading \"%s\" it's not currently used", name.c_str());
            return false;
        }

        X_LOG0("WeaponDef", "Reloading: %s", name.c_str());

        pAssetLoader_->reload(pWeaponRes, core::ReloadFlag::Beginframe);
        return true;
    }

    void WeaponDefManager::listWeapons(core::string_view searchPattern) const
    {
        core::ScopedLock<WeaponDefContainer::ThreadPolicy> lock(weaponDefs_.getThreadPolicy());

        core::Array<WeaponDefResource*> sorted(arena_);
        weaponDefs_.getSortedAssertList(sorted, searchPattern);

        X_LOG0("WeaponDef", "------------ ^8Weapons(%" PRIuS ")^7 ---------------", sorted.size());

        for (const auto* pWpn : sorted) {
            X_LOG0("WeaponDef", "^2%-32s^7 Refs: ^2%i",
                pWpn->getName().c_str(), pWpn->getRefCount());
        }

        X_LOG0("WeaponDef", "------------ ^8Weapnos End^7 --------------");
    }


    void WeaponDefManager::Cmd_List(core::IConsoleCmdArgs* pCmd)
    {
        core::string_view searchPattern;

        if (pCmd->GetArgCount() >= 2) {
            searchPattern = pCmd->GetArg(1);
        }

        listWeapons(searchPattern);
    }

} // namespace weapon

X_NAMESPACE_END