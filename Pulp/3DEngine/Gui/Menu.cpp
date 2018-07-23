#include "stdafx.h"
#include "Menu.h"

#include <IRender.h>
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(engine)


namespace gui
{

    XMenu::XMenu(core::string& name) :
        core::AssetBase(name, assetDb::AssetType::MENU)
    {
    }

    XMenu::~XMenu()
    {
    }

    void XMenu::draw(engine::IPrimativeContext* pDrawCon)
    {
        X_UNUSED(pDrawCon);

        // o baby.
        // so i basically want to run the script.

    }

    bool XMenu::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        data_ = std::move(data);
        dataSize_ = dataSize;
        return true;
    }
  

} // namespace gui

X_NAMESPACE_END