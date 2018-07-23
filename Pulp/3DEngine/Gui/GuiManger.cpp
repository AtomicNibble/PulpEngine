#include "stdafx.h"
#include "GuiManger.h"

#include <Assets\AssetLoader.h>

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

        pAssetLoader_ = gEnv->pCore->GetAssetLoader();
        pAssetLoader_->registerAssetType(assetDb::AssetType::MENU, this, MENU_FILE_EXTENSION);

        return true;
    }

    void XGuiManager::shutdown(void)
    {
        X_LOG0("GuiManager", "Shutting Down");


    }

    IGui* XGuiManager::loadGui(const char* pName)
    {
        X_ASSERT(core::strUtil::FileExtension(pName) == nullptr, "Extension not allowed")(pName);

        core::string name(pName);
        core::ScopedLock<MenuContainer::ThreadPolicy> lock(menus_.getThreadPolicy());

        auto* pMenu = menus_.findAsset(name);
        if (pMenu) {
            // inc ref count.
            pMenu->addReference();
            return pMenu;
        }

        // we create a model and give it back
        pMenu = menus_.createAsset(name, name);

        addLoadRequest(pMenu);

        return pMenu;
    }

    IGui* XGuiManager::findGui(const char* pName)
    {
        core::string name(pName);
        core::ScopedLock<MenuContainer::ThreadPolicy> lock(menus_.getThreadPolicy());

        auto* pMenu = menus_.findAsset(name);
        if (pMenu) {
            return pMenu;
        }

        X_WARNING("GuiManager", "Failed to find menu: \"%s\"", pName);
        return nullptr;
    }

    void XGuiManager::releaseGui(IGui* pGui)
    {
        MenuResource* pGuiRes = static_cast<MenuResource*>(pGui);
        if (pGuiRes->removeReference() == 0) {

            menus_.releaseAsset(pGuiRes);
        }
    }

    bool XGuiManager::waitForLoad(IGui* pIGui)
    {
        auto* pGui = static_cast<XGui*>(pIGui);
        if (pGui->getStatus() == core::LoadStatus::Complete) {
            return true;
        }

        return pAssetLoader_->waitForLoad(pGui);
    }

    void XGuiManager::addLoadRequest(MenuResource* pMenu)
    {
        pAssetLoader_->addLoadRequest(pMenu);
    }

    void XGuiManager::onLoadRequestFail(core::AssetBase* pAsset)
    {
        X_UNUSED(pAsset);
    }

    bool XGuiManager::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        auto* pMenu = static_cast<XGui*>(pAsset);

        return pMenu->processData(std::move(data), dataSize);
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