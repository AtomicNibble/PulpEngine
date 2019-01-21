#include "stdafx.h"
#include "LightSys.h"

#include <IWorld3D.h>

X_NAMESPACE_BEGIN(game)

namespace entity
{
    LightSystem::LightSystem() :
        p3DWorld_(nullptr)
    {

    }

    bool LightSystem::init(ECS& reg, engine::IWorld3D* p3DWorld)
    {
        reg.registerHandler<LightSystem, MsgMove>(this);
        p3DWorld_ = p3DWorld;
        return true;
    }

    void LightSystem::update(const core::FrameData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);
    }

    void LightSystem::onMsg(ECS& reg, const MsgMove& msg)
    {
        if (!reg.has<Light>(msg.id)) {
            return;
        }

        auto trans = reg.get<TransForm>(msg.id);
        auto& light = reg.get<Light>(msg.id);

        trans.pos += trans.quat * light.offset;

        p3DWorld_->updateLight(light.pLight, trans);
    }

} // namespace entity

X_NAMESPACE_END
