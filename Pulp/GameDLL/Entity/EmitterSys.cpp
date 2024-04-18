#include "stdafx.h"
#include "EmitterSys.h"

#include <IEffect.h>


X_NAMESPACE_BEGIN(game)

namespace entity
{
    EmitterSys::EmitterSys()
    {
        
    }

    bool EmitterSys::init(ECS& reg)
    {
        reg.registerHandler<EmitterSys, MsgMove>(this);
        return true;
    }

    void EmitterSys::update(core::FrameData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);
    }

    void EmitterSys::onMsg(ECS& reg, const MsgMove& msg)
    {
        if (!reg.has<Emitter>(msg.id)) {
            return;
        }

        auto& trans = reg.get<TransForm>(msg.id);
        auto& emt = reg.get<Emitter>(msg.id);

        emt.pEmitter->setTrans(trans, emt.offset);
    }

} // namespace entity

X_NAMESPACE_END