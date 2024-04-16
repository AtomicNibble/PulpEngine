#pragma once

#ifndef _X_RENDER_SYS_H_
#define _X_RENDER_SYS_H_

#include "I3DEngine.h"
#include "Vars\DrawVars.h"

#include <IModel.h>
#include <IRenderMesh.h>

#include "Drawing\PrimitiveContext.h"

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs)

X_NAMESPACE_DECLARE(anim,
                    class AnimManager)


X_NAMESPACE_BEGIN(engine)

X_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier

class TextureManager;

namespace fx
{
    class EffectManager;
} // namespace fx

class X3DEngine : public I3DEngine
    , public ICoreEventListener
{
    typedef core::Array<IWorld3D*> WorldArr;

    typedef core::FixedArray<PrimitiveContext*, PrimContext::ENUM_COUNT> PrimitiveContextArr;

    X_DECLARE_ENUM(PixelBuf)(
        DEPTH_STENCIL,
        COL_3D,
        COL_2D
    );

    typedef std::array<render::IPixelBuffer*, PixelBuf::ENUM_COUNT> PixelBufferArr;

public:
    X3DEngine(core::MemoryArenaBase* arena);
    virtual ~X3DEngine() X_FINAL;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    bool asyncInitFinalize(void) X_FINAL;
    void clearPersistent(void) X_FINAL;

    void update(core::FrameData& frame) X_FINAL;
    void onFrameBegin(core::FrameData& frame) X_FINAL;

    IPrimitiveContext* getPrimContext(PrimContext::Enum user) X_FINAL;
    IMaterialManager* getMaterialManager(void) X_FINAL;
    model::IModelManager* getModelManager(void) X_FINAL;
    anim::IAnimManager* getAnimManager(void) X_FINAL;
    fx::IEffectManager* getEffectManager(void) X_FINAL;
    gui::IMenuManager* getMenuManager(void) X_FINAL;

    IWorld3D* create3DWorld(physics::IScene* pPhysScene) X_FINAL;
    void release3DWorld(IWorld3D* pWorld) X_FINAL;
    void addWorldToActiveList(IWorld3D* pWorld) X_FINAL;
    void removeWorldFromActiveList(IWorld3D* pWorld) X_FINAL;

private:
    void renderPrimContex2D(core::FrameData& frame, IPrimitiveContext::Mode mode);
    void addPrimsToBucket(core::FrameData& frame, render::CommandBucket<uint32_t>& primBucket,
        IPrimitiveContext::Mode mode, core::span<PrimitiveContext*> prims);

    void getPrimsWithData(PrimitiveContextArr& prims, IPrimitiveContext::Mode mode);
    static size_t getTotalElems(const PrimitiveContextArr& prims);

private:
    void OnCoreEvent(const CoreEventData& ed) X_FINAL;

private:
    void Command_ClearPersistent(core::IConsoleCmdArgs* pCmd);
    void Command_WriteBufferToFile(core::IConsoleCmdArgs* pCmd);

private:
    XMaterialManager* pMaterialManager_;
    TextureManager* pTextureManager_;
    model::XModelManager* pModelManager_;
    anim::AnimManager* pAnimManager_;
    fx::EffectManager* pEffectManager_;

    gui::XMenuManager* pMenuManager_;

    CBufferManager* pCBufMan_;
    VariableStateManager* pVariableStateMan_;

    // ---
    PrimitiveContextSharedResources primResources_;
    PrimitiveContext primContexts_[PrimContext::ENUM_COUNT];

    PixelBufferArr pixelBufffers_;

    bool clearPersistent_;
    bool coreEventReg_;

    DrawVars drawVars_;
    WorldArr worlds_;
};

X_ENABLE_WARNING(4324) //  structure was padded due to alignment specifier

X_NAMESPACE_END

#endif // !_X_RENDER_SYS_H_
