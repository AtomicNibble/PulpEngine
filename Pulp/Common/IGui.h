#pragma once

#ifndef X_GUI_H_I_
#define X_GUI_H_I_

#include <Math\XVector.h>

X_NAMESPACE_DECLARE(core, struct FrameData);
X_NAMESPACE_DECLARE(net, struct ISession);

X_NAMESPACE_BEGIN(engine)

class IPrimativeContext;

namespace gui
{
    static const char* MENU_FILE_EXTENSION = "lua"; // HOT DAM!

    static const uint32_t MENU_MAX_LOADED = 64;

    struct MenuParams
    {
        net::ISession* pSession;
    };

    struct IMenu
    {
        virtual ~IMenu() = default;

    };

    struct IMenuHandler
    {
        virtual ~IMenuHandler() = default;

        virtual bool isActive(void) const X_ABSTRACT;
        virtual void update(MenuParams& params, core::FrameData& frame, IPrimativeContext* pDrawCon) X_ABSTRACT;

        virtual bool openMenu(core::string_view name) X_ABSTRACT;
        virtual void close(void) X_ABSTRACT;
        virtual bool back(bool close) X_ABSTRACT;
    };

    struct IMenuManager
    {
        virtual ~IMenuManager() = default;

        virtual IMenuHandler* createMenuHandler(void) X_ABSTRACT;
        virtual void releaseMenuHandler(IMenuHandler* pHandler) X_ABSTRACT;
        virtual IMenu* loadMenu(core::string_view name) X_ABSTRACT;
        virtual IMenu* findMenu(core::string_view name) X_ABSTRACT;

        virtual void releaseGui(IMenu* pMenu) X_ABSTRACT;

        virtual bool waitForLoad(IMenu* pMenu) X_ABSTRACT;

        virtual void listGuis(core::string_view searchPattern) const X_ABSTRACT;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_H_I_
