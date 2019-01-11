#pragma once

#include <IWeapon.h>

#include <Assets\AssertContainer.h>
#include <IAsyncLoad.h>

#include "WeaponDef.h"

X_NAMESPACE_DECLARE(core,
                    namespace V2 {
                        struct Job;
                        class JobSystem;
                    }
                    struct IConsoleCmdArgs;
                    class AssetLoader;)


X_NAMESPACE_BEGIN(game)

namespace weapon
{
    class WeaponDef;

    class WeaponDefManager : public core::IAssetLoader
        , private core::IAssetLoadSink
    {
        typedef core::AssetContainer<WeaponDef, WEAPON_MAX_LOADED, core::SingleThreadPolicy> WeaponDefContainer;
        typedef WeaponDefContainer::Resource WeaponDefResource;

        typedef WeaponDefContainer AssetContainer;

        typedef core::Array<core::StrHash> StrHashArr;

    public:
        WeaponDefManager(core::MemoryArenaBase* arena);

        void registerCmds(void);
        void registerVars(void);

        bool init(void);
        void shutDown(void);

        bool asyncInitFinalize(void);

        WeaponDef* findWeaponDef(core::AssetID id) const;
        WeaponDef* findWeaponDef(core::string_view name) const;
        WeaponDef* loadWeaponDef(core::string_view name);
        WeaponDef* getDefaultDef(void) const;

        bool waitForLoad(core::AssetBase* pWeaponDef) X_FINAL;
        bool waitForLoad(WeaponDef* pWeaponDef); // returns true if load succeed.
        void releaseWeaponDef(WeaponDef* pWeaponDef);

        void listWeapons(const char* pSearchPatten = nullptr) const;

        AmmoTypeId getAmmoTypeId(const char* pName);


    private:
        bool initDefaults(void);
        void freeDangling(void);
        void releaseResources(WeaponDef* pWeaponDef);

        void addLoadRequest(WeaponDefResource* pWeaponDef);
        void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
        bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;
        bool onFileChanged(const core::AssetName& assetName, const core::string& name) X_FINAL;

    private:
        void Cmd_List(core::IConsoleCmdArgs* pCmd);

    private:
        core::MemoryArenaBase* arena_;
        core::AssetLoader* pAssetLoader_;

        WeaponDef* pDefaultWeaponDef_;

        WeaponDefContainer weaponDefs_;

        StrHashArr ammoTypeList_;
        core::CriticalSection cs_;
    };

} // namespace weapon

X_NAMESPACE_END