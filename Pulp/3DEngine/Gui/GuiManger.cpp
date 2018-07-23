#include "stdafx.h"
#include "GuiManger.h"

#include <IConsole.h>
#include <IRender.h>

#include "Material\MaterialManager.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{

    XGuiManager::XGuiManager(core::MemoryArenaBase* arena, XMaterialManager* pMatMan) :
        arena_(arena),
        pMatMan_(pMatMan),
        menus_(arena, sizeof(MenuResource), X_ALIGN_OF(MenuResource), "MenuPool")
    {
        

    }

    XGuiManager::~XGuiManager()
    {

    }

    bool XGuiManager::init(void)
    {
        X_LOG0("GuiManager", "Starting GUI System");

        ADD_COMMAND_MEMBER("listUi", this, XGuiManager, &XGuiManager::Cmd_ListUis, core::VarFlags::SYSTEM, "List the loaded ui");


        return true;
    }

    void XGuiManager::shutdown(void)
    {
        X_LOG0("GuiManager", "Shutting Down");


    }

    IGui* XGuiManager::loadGui(const char* pName)
    {
        X_UNUSED(pName);
        return nullptr;
    }

    IGui* XGuiManager::findGui(const char* pName)
    {
        core::string name(pName);
        core::ScopedLock<MenuContainer::ThreadPolicy> lock(menus_.getThreadPolicy());

        auto* pMenu = menus_.findAsset(name);
        if (pMenu) {
            return pMenu;
        }

        X_WARNING("GuiManager", "Failed to find model: \"%s\"", pName);
        return nullptr;
    }

    void XGuiManager::listGuis(const char* pWildcardSearch) const
    {
        X_UNUSED(pWildcardSearch);
    }


    void XGuiManager::Cmd_ListUis(core::IConsoleCmdArgs* pArgs)
    {
        // we support wildcards
        const char* pSearchString = nullptr;
        if (pArgs->GetArgCount() > 1) {
            pSearchString = pArgs->GetArg(1);
        }

        listGuis(pSearchString);
    }

} // namespace gui

X_NAMESPACE_END