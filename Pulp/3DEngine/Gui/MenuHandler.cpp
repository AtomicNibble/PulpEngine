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

    void MenuHandler::init(engine::Material* pCursor)
    {
        ctx_.init(pCursor);

        pMenu_ = static_cast<Menu*>(gEngEnv.pMenuMan_->loadMenu("pause"));
    }

    void MenuHandler::update(core::FrameData& frame, IPrimativeContext* pIPrim)
    {
        auto* pPrim = static_cast<PrimativeContext*>(pIPrim);

        // auto r = frame.view.viewport.getRect();

        GuiContex::Params p;
        p.rect.set(0.f, 0.f, 1680.f, 1050.f);
        p.cursorPos = frame.input.cusorPosClient;
        p.frameDelta = frame.timeInfo.deltas[core::Timer::UI];

        ctx_.setPrimContet(pPrim);
        ctx_.setFont(gEnv->pFontSys->getDefault());
        ctx_.processInput(frame.input);
        ctx_.begin(p);


        // if menu active.. blar.. blar..
        if(pMenu_->isLoaded())
        {
            // I want to tickle a goat.
            // but alas my pickle jar is empty!
            // if not for the camel under the tree, i would wiggle my knee.
            // tis a problem you see!
            // 
            // auto* pScriptSys = gEnv->pScriptSys;

            // draw menu.
            if (ctx_.button("RESUME"))
            {
                // do stuff
                X_LOG0("Game", "Resume Game!");
            }

            if (ctx_.button("OPTIONS"))
            {
                // do stuff, basically i just want to change the active menu.
                // and the menu manager should just keep a navigation stack?
                X_LOG0("Game", "Options menu");
            }

            if (ctx_.button("QUIT"))
            {
                // do stuff
                X_LOG0("Game", "Quit why :( ?");
            }

            ctx_.slider("VOLUME", "snd_vol_master");
        }

        ctx_.end();
    }

} // namespace gui

X_NAMESPACE_END
