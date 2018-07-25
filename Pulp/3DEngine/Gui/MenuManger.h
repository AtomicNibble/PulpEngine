#pragma once

#include <IGui.h>

#include "Menu.h"

#include <Assets\AssertContainer.h>
#include <IAsyncLoad.h>

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs);

X_NAMESPACE_DECLARE(engine,
    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class ScriptBinds_Menu;

    class XMenuManager : public IMenuManager
        , private core::IAssetLoadSink
    {
        typedef core::AssetContainer<XMenu, MENU_MAX_LOADED, core::SingleThreadPolicy> MenuContainer;
        typedef MenuContainer::Resource MenuResource;

    public:
        XMenuManager(core::MemoryArenaBase* arena, XMaterialManager* pMatMan);
        ~XMenuManager() X_FINAL;

        //IMenuManager
        bool init(void) X_FINAL;
        void shutdown(void) X_FINAL;

        IMenu* loadMenu(const char* pName) X_FINAL;
        IMenu* findMenu(const char* pName) X_FINAL;

        void releaseGui(IMenu* pMenu) X_FINAL;
        bool waitForLoad(IMenu* pMenu) X_FINAL;

        void listGuis(const char* pWildcardSearch = nullptr) const X_FINAL;
        //~IMenuManager

        void draw(IPrimativeContext* pPrim, IMenu* pMenu);

    private:
        // load / processing
        void addLoadRequest(MenuResource* pMenu);
        void onLoadRequestFail(core::AssetBase* pAsset) X_FINAL;
        bool processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_FINAL;

    private:
        void Cmd_ListUis(core::IConsoleCmdArgs* pArgs);

    private:
        core::MemoryArenaBase* arena_;
        XMaterialManager* pMatMan_;
        script::IScriptSys* pScriptSys_;
        core::AssetLoader* pAssetLoader_;
    
        ScriptBinds_Menu* pScriptBinds_;

        MenuContainer menus_;
    };

} // namespace gui

X_NAMESPACE_END
