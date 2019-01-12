#pragma once

#include <IGui.h>

#include "GuiContex.h"

X_NAMESPACE_DECLARE(core, struct FrameData)
X_NAMESPACE_DECLARE(net, struct ISession)

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
        MenuHandler(GuiContex& ctx, XMenuManager& man);
        ~MenuHandler() X_OVERRIDE = default;

        bool isActive(void) const X_FINAL;
        void update(MenuParams& params, core::FrameData& frame, IPrimativeContext* pPrim) X_FINAL;

        bool openMenu(core::string_view name) X_FINAL;
        void close(void) X_FINAL;
        bool back(bool close) X_FINAL;

    private:
        GuiContex& ctx_;
        XMenuManager& man_;

        bool firstMenu_;
        Menu* pActiveMenu_;
        MenuStack stack_;
    };


} // namespace gui

X_NAMESPACE_END
