#include "stdafx.h"
#include "MeshRendererSys.h"

#include <IWorld3D.h>


X_NAMESPACE_BEGIN(game)

namespace entity
{
    MeshRendererSys::MeshRendererSys() :
        p3DWorld_(nullptr)
    {
    }

    bool MeshRendererSys::init(ECS& reg, engine::IWorld3D* p3DWorld)
    {
        reg.registerHandler<MeshRendererSys, MsgMove>(this);
        p3DWorld_ = p3DWorld;
        return true;
    }

    void MeshRendererSys::update(core::FrameData& frame, ECS& reg)
    {
        X_UNUSED(frame, reg);
    }

    void MeshRendererSys::onMsg(ECS& reg, const MsgMove& msg)
    {
        if (!reg.has<MeshRenderer>(msg.id)) {
            return;
        }

        auto& trans = reg.get<TransForm>(msg.id);
        auto& rendEnt = reg.get<MeshRenderer>(msg.id);

        p3DWorld_->updateRenderEnt(rendEnt.pRenderEnt, trans);
    }

} // namespace entity

X_NAMESPACE_END