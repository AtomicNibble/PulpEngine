#pragma once

#include <IGui.h>

#include "Gui.h"

#include <Assets\AssertContainer.h>
#include <IAsyncLoad.h>

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XGuiManager : public IGuiManger
    {
        typedef core::AssetContainer<XGui, MENU_MAX_LOADED, core::SingleThreadPolicy> MenuContainer;
        typedef MenuContainer::Resource MenuResource;

    public:
        XGuiManager(core::MemoryArenaBase* arena, XMaterialManager* pMatMan);
        ~XGuiManager() X_FINAL;

        //IGuiManger
        bool init(void) X_FINAL;
        void shutdown(void) X_FINAL;

        IGui* loadGui(const char* pName) X_FINAL;
        IGui* findGui(const char* pName) X_FINAL;

        void listGuis(const char* pWildcardSearch = nullptr) const X_FINAL;
        //~IGuiManger

    private:
        void Cmd_ListUis(core::IConsoleCmdArgs* pArgs);

    private:
        core::MemoryArenaBase* arena_;
        XMaterialManager* pMatMan_;

    
        MenuContainer menus_;
    };

} // namespace gui

X_NAMESPACE_END
