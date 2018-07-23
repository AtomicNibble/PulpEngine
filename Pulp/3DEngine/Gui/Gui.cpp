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

    void XGui::Draw(engine::IPrimativeContext* pDrawCon)
    {
        X_UNUSED(pDrawCon);

    }
  

} // namespace gui

X_NAMESPACE_END