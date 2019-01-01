#include "stdafx.h"
#include "SoundSys.h"

X_NAMESPACE_BEGIN(game)

namespace entity
{
    SoundSystem::SoundSystem()
    {
        pSound_ = gEnv->pSound;
    }

    bool SoundSystem::init(ECS& reg)
    {
        reg.registerHandler<SoundSystem, MsgMove>(this);
        return true;
    }


    void SoundSystem::update(core::FrameData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);
    }

    void SoundSystem::onMsg(ECS& reg, const MsgMove& msg)
    {
        if (!reg.has<SoundObject>(msg.id)) {
            return;
        }

        auto trans = reg.get<TransForm>(msg.id); // copy
        auto& snd = reg.get<SoundObject>(msg.id);

        trans.pos += snd.offset;

        pSound_->setPosition(snd.handle, trans);
    }

} // namespace entity

X_NAMESPACE_END