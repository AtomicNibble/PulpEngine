#pragma once

#ifndef X_GUI_H_I_
#define X_GUI_H_I_

#include <Math\XVector.h>


X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    static const char* MENU_FILE_EXTENSION = "lua";

    static const uint32_t MENU_MAX_LOADED = 64;



    struct IMenu
    {
        virtual ~IMenu() = default;

        virtual void draw(engine::IPrimativeContext* pDrawCon) X_ABSTRACT;
    };

    struct IMenuManager
    {
        virtual ~IMenuManager() = default;

        virtual bool init(void) X_ABSTRACT;
        virtual void shutdown(void) X_ABSTRACT;

        virtual IMenu* loadMenu(const char* pName) X_ABSTRACT;
        virtual IMenu* findMenu(const char* pName) X_ABSTRACT;

        virtual void releaseGui(IMenu* pMenu) X_ABSTRACT;

        virtual bool waitForLoad(IMenu* pMenu) X_ABSTRACT;

        virtual void listGuis(const char* pWildcardSearch = nullptr) const X_ABSTRACT;
    };

} // namespace gui

X_NAMESPACE_END

#endif // !X_GUI_H_I_
