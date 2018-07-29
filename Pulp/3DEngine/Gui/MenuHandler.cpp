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
    
    MenuHandler::MenuHandler(GuiContex& ctx) :
        ctx_(ctx)
    {

    }

    void MenuHandler::init(engine::Material* pCursor)
    {
        ctx_.init(pCursor);

        auto* pMenu = static_cast<Menu*>(gEngEnv.pMenuMan_->loadMenu("pause"));

        stack_.push(pMenu);
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

        // if menu active.. blar.. blar..
        auto* pMenu = stack_.top();
        if(pMenu->isLoaded())
        {
            pMenu->draw();
        }

        ctx_.end();
    }

    void MenuHandler::openMenu(const char* pName)
    {
        X_UNUSED(pName);

        auto* pMenu = static_cast<Menu*>(gEngEnv.pMenuMan_->loadMenu(pName));
       
        stack_.push(pMenu);
    }

    void MenuHandler::closeMenu(void)
    {
        stack_.clear();
    }

    void MenuHandler::back(void)
    {
        if (!stack_.isEmpty()) {
            stack_.pop();
        }
    }


} // namespace gui

X_NAMESPACE_END
