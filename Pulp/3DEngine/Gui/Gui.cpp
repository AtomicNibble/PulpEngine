#include "stdafx.h"
#include "Gui.h"

#include <IRender.h>
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(engine)


namespace gui
{

    XGui::XGui(core::string& name) :
        core::AssetBase(name, assetDb::AssetType::MENU)
    {
    }

    XGui::~XGui()
    {
    }

    void XGui::draw(engine::IPrimativeContext* pDrawCon)
    {
        X_UNUSED(pDrawCon);

        // o baby.
        // so i basically want to run the script.

    }

    bool XGui::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        data_ = std::move(data);
        dataSize_ = dataSize;
        return true;
    }
  

} // namespace gui

X_NAMESPACE_END