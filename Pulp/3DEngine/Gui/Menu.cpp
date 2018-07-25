#include "stdafx.h"
#include "Menu.h"
#include "MenuManger.h"

#include <IScriptSys.h>

X_NAMESPACE_BEGIN(engine)


namespace gui
{

    XMenu::XMenu(XMenuManager& menuMan, core::string& name) :
        core::AssetBase(name, assetDb::AssetType::MENU),
        menuMan_(menuMan)
    {
    }

    XMenu::~XMenu()
    {
    }

    void XMenu::draw(engine::IPrimativeContext* pDrawCon)
    {
        X_UNUSED(pDrawCon);

        auto* pScriptSys = gEnv->pScriptSys;

        const char* pBegin = data_.ptr();
        const char* pEnd = pBegin + dataSize_;


        pScriptSys->runScriptInSandbox(pBegin, pEnd);

        // o baby.
        // so i basically want to run the script.
        menuMan_.draw(pDrawCon, this);
    }

    bool XMenu::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        data_ = std::move(data);
        dataSize_ = dataSize;
        return true;
    }
  

} // namespace gui

X_NAMESPACE_END