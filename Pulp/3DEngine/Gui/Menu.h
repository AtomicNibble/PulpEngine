#pragma once

#include <IGui.h>

#include <Assets\AssetBase.h>

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XMenuManager;

    class XMenu : public IMenu, public core::AssetBase
    {
    public:
        XMenu(XMenuManager& menuMan, core::string& name);
        ~XMenu() X_OVERRIDE;

        void draw(engine::IPrimativeContext* pDrawCon) X_FINAL;

        bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

    private:
        XMenuManager& menuMan_;
        core::UniquePointer<char[]> data_;
        uint32_t dataSize_;
    };


} // namespace gui

X_NAMESPACE_END

#include "Menu.inl"
