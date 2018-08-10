#pragma once

#include <IGui.h>
#include <IScriptSys.h>

#include <Assets\AssetBase.h>

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    class XMenuManager;

    class Menu : public IMenu, public core::AssetBase
    {
    public:
        Menu(core::string& name);
        ~Menu() X_OVERRIDE;

        void draw(void);

        bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

    private:
        core::UniquePointer<char[]> data_;
        uint32_t dataSize_;

        script::IScriptTable* pScriptTable_;
        script::ScriptValue updateFunc_;
    };


} // namespace gui

X_NAMESPACE_END

#include "Menu.inl"