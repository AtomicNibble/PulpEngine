#pragma once

#ifndef _X_RENDER_SYS_H_
#define _X_RENDER_SYS_H_

#include "I3DEngine.h"
#include "Vars\DrawVars.h"

#include <IModel.h>
#include <IRenderMesh.h>

#include "Drawing\PrimativeContext.h"

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs;)

X_NAMESPACE_DECLARE(anim,
                    class AnimManager;)

X_NAMESPACE_BEGIN(engine)

X_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier

class TextureManager;

namespace fx
{
    class EffectManager;
} // namespace fx

class X3DEngine : public I3DEngine
    , public core::IXHotReload
{
    typedef core::Array<IWorld3D*> WorldArr;

public:
    X3DEngine(core::MemoryArenaBase* arena);
    virtual ~X3DEngine() X_FINAL;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    bool asyncInitFinalize(void) X_FINAL;

    void Update(core::FrameData& frame) X_FINAL;
    void OnFrameBegin(core::FrameData& frame) X_FINAL;

    IPrimativeContext* getPrimContext(PrimContext::Enum user) X_FINAL;
    IMaterialManager* getMaterialManager(void) X_FINAL;
    model::IModelManager* getModelManager(void) X_FINAL;
    anim::IAnimManager* getAnimManager(void) X_FINAL;
    fx::IEffectManager* getEffectManager(void) X_FINAL;

    IWorld3D* create3DWorld(physics::IScene* pPhysScene) X_FINAL;
    void release3DWorld(IWorld3D* pWorld) X_FINAL;
    void addWorldToActiveList(IWorld3D* pWorld) X_FINAL;
    void removeWorldFromActiveList(IWorld3D* pWorld) X_FINAL;

    // IXHotReload
    void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
    // ~IXHotReload

private:
    void Command_ClearPersistent(core::IConsoleCmdArgs* pCmd);

private:
    XMaterialManager* pMaterialManager_;
    TextureManager* pTextureManager_;
    model::XModelManager* pModelManager_;
    anim::AnimManager* pAnimManager_;
    fx::EffectManager* pEffectManager_;

    gui::XGuiManager* pGuiManger_;

    CBufferManager* pCBufMan_;
    VariableStateManager* pVariableStateMan_;

    // ---
    PrimativeContextSharedResources primResources_;
    PrimativeContext primContexts_[PrimContext::ENUM_COUNT];

    bool clearPersistent_;

    DrawVars drawVars_;
    WorldArr worlds_;
};

X_ENABLE_WARNING(4324) //  structure was padded due to alignment specifier

X_NAMESPACE_END

#endif // !_X_RENDER_SYS_H_
