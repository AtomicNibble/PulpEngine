#include "stdafx.h"
#include "MenuHandler.h"

#include "Menu.h"
#include "MenuManger.h"
#include "GuiContex.h"
#include "Drawing\PrimativeContext.h"

#include <IFrameData.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    
    MenuHandler::MenuHandler(GuiContex& ctx, XMenuManager& man) :
        ctx_(ctx),
        man_(man),
        pActiveMenu_(nullptr),
        firstMenu_(false)
    {

    }

    bool MenuHandler::isActive(void) const
    {
        return !stack_.isEmpty();
    }

    void MenuHandler::update(core::FrameData& frame, IPrimativeContext* pIPrim)
    {
        auto* pPrim = static_cast<PrimativeContext*>(pIPrim);

        if (stack_.isEmpty()) {
            return;
        }

        GuiContex::Params p;
        p.rect.set(0.f, 0.f, 1680.f, 1050.f);
        p.cursorPos = frame.input.cusorPosClient;
        p.frameDelta = frame.timeInfo.deltas[core::Timer::UI];

        ctx_.setPrimContet(pPrim);
        ctx_.setFont(gEnv->pFontSys->getDefault());
        ctx_.processInput(frame.input);
        ctx_.begin(p);

        man_.setActiveHandler(this);

        // if menu active.. blar.. blar..
        auto* pMenu = stack_.top();
        if(pMenu->isLoaded())
        {
            if (pActiveMenu_ != pMenu)
            {
                if(firstMenu_)
                { 
                    firstMenu_ = false;
                    pMenu->onOpen();
                }

                pActiveMenu_ = pMenu;
            }

            pMenu->draw(frame);
        }

#if X_SUPER == 0
        man_.setActiveHandler(nullptr);
#endif // !X_SUPER

        ctx_.end();
    }

    bool MenuHandler::openMenu(const char* pName)
    {
        auto* pMenu = static_cast<Menu*>(gEngEnv.pMenuMan_->loadMenu(pName));
        if (!pMenu) {
            return false;
        }

        if (stack_.isEmpty()) {
            firstMenu_ = true;
        }

        stack_.push(pMenu);
        return true;
    }

    void MenuHandler::closeMenu(void)
    {
        pActiveMenu_ = nullptr;
        stack_.clear();
    }

    bool MenuHandler::back(void)
    {
        if (stack_.isEmpty()) {
            return false;
        }

        pActiveMenu_ = nullptr;
        stack_.pop();
        return true;
    }


} // namespace gui

X_NAMESPACE_END
