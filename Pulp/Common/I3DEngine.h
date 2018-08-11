#pragma once

#ifndef _X_RENDER_SYS_I_H_
#define _X_RENDER_SYS_I_H_

X_NAMESPACE_DECLARE(physics,
    struct IScene)

X_NAMESPACE_DECLARE(model,
    struct IModelManager)

X_NAMESPACE_DECLARE(anim,
    struct IAnimManager)


X_NAMESPACE_BEGIN(engine)


X_DECLARE_ENUM(PrimContext)
(
    SOUND,      // 3d
    PHYSICS,    // 3d
    MISC3D,     // 3d
    PERSISTENT, // 3d
    GUI,        // 2d
    PROFILE,    // 2d
    CONSOLE     // 2d
);

class IPrimativeContext;
struct IMaterialManager;
struct IWorld3D;

namespace fx
{
    struct IEffectManager;
} // namespace fx

namespace gui 
{
    struct IMenuManager;
} // namespace gui

struct I3DEngine : public core::IEngineSysBase
{
    virtual ~I3DEngine() = default;

    // finish any async init tasks for all fonts.
    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    virtual void update(core::FrameData& frame) X_ABSTRACT;
    virtual void onFrameBegin(core::FrameData& frame) X_ABSTRACT;

    // each enum has a instance, and you don't own the pointer.
    virtual IPrimativeContext* getPrimContext(PrimContext::Enum user) X_ABSTRACT;
    virtual IMaterialManager* getMaterialManager(void) X_ABSTRACT;
    virtual model::IModelManager* getModelManager(void) X_ABSTRACT;
    virtual anim::IAnimManager* getAnimManager(void) X_ABSTRACT;
    virtual fx::IEffectManager* getEffectManager(void) X_ABSTRACT;
    virtual gui::IMenuManager* getMenuManager(void) X_ABSTRACT;

    virtual IWorld3D* create3DWorld(physics::IScene* pPhysScene) X_ABSTRACT;
    virtual void release3DWorld(IWorld3D* pWorld) X_ABSTRACT;
    virtual void addWorldToActiveList(IWorld3D* pWorld) X_ABSTRACT;
    virtual void removeWorldFromActiveList(IWorld3D* pWorld) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !_X_RENDER_SYS_I_H_
