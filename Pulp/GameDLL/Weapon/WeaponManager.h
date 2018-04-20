#pragma once

#include <IWeapon.h>
#include <IDirectoryWatcher.h>

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
        , public core::IXHotReload
        , private core::IAssetLoadSink
    {
        typedef core::AssetContainer<WeaponDef, WEAPON_MAX_LOADED, core::SingleThreadPolicy> WeaponDefContainer;
        typedef WeaponDefContainer::Resource WeaponDefResource;

    public:
        WeaponDefManager(core::MemoryArenaBase* arena);

        void registerCmds(void);
        void registerVars(void);

        bool init(void);
        void shutDown(void);

        bool asyncInitFinalize(void);

        WeaponDef* findWeaponDef(const char* pName) const;
        WeaponDef* loadWeaponDef(const char* pName);
        WeaponDef* getDefaultDef(void) const;

        bool waitForLoad(core::AssetBase* pWeaponDef) X_FINAL;
        bool waitForLoad(WeaponDef* pWeaponDef); // returns true if load succeed.
        void releaseWeaponDef(WeaponDef* pWeaponDef);

        void listWeapons(const char* pSearchPatten = nullptr) const;

    private:
        bool initDefaults(void);
        void freeDangling(void);

        void addLoadRequest(WeaponDefResource* pWeaponDef);
        void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
        bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;

    private:
        // IXHotReload
        virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
        // ~IXHotReload

        void Cmd_List(core::IConsoleCmdArgs* pCmd);

    private:
        core::MemoryArenaBase* arena_;
        core::AssetLoader* pAssetLoader_;

        WeaponDef* pDefaultWeaponDef_;

        WeaponDefContainer weaponDefs_;
    };

} // namespace weapon

X_NAMESPACE_END