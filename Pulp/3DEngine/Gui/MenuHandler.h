#pragma once

#include <IGui.h>

#include "GuiContex.h"

X_NAMESPACE_DECLARE(core, struct FrameData)

X_NAMESPACE_BEGIN(engine)

class IPrimativeContext;

namespace gui
{
    class Menu;
    class GuiContex;

    // This is like a active menu context.
    // it's state bsed and handles switching between menus.
    class MenuHandler : public IMenuHandler
    {
        using MenuStack = core::FixedStack<Menu*, 8>;

    public:
        MenuHandler(GuiContex& ctx);
        ~MenuHandler() X_OVERRIDE = default;

        void init(engine::Material* pCursor);

        bool open(const char* pName) X_FINAL;
        bool isActive(void) const X_FINAL;
        void update(core::FrameData& frame, IPrimativeContext* pPrim) X_FINAL;

        void openMenu(const char* pName);
        void closeMenu(void);
        void back(void);

    private:
        GuiContex& ctx_;

       // Menu* pMenu_;

        MenuStack stack_;
    };


} // namespace gui

X_NAMESPACE_END
