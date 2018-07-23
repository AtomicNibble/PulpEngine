#pragma once

#include <IGui.h>

#include <Assets\AssetBase.h>

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XGui : public IGui, core::AssetBase
    {
    public:
        XGui(core::string& name);
        ~XGui() X_OVERRIDE;

        void Draw(engine::IPrimativeContext* pDrawCon) X_FINAL;

    private:

    };


} // namespace gui

X_NAMESPACE_END

#include "Gui.inl"
