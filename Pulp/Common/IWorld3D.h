#pragma once

#include <ILevel.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    class Effect;
    struct IEmitter;
} // namespace fx

struct RenderEntDesc
{
    model::XModel* pModel;

    Transformf trans;
};

struct RenderLightDesc
{
    Transformf trans;

    Colorf col;
};

struct EmitterDesc
{
    Transformf trans;
    bool looping;

    fx::Effect* pEffect;
};

struct IRenderEnt
{
};

struct IRenderLight
{
};

struct IWorld3D
{
    virtual ~IWorld3D() = default;

    virtual bool loadNodes(const level::FileHeader& fileHdr, level::StringTable& strTable, uint8_t* pData) X_ABSTRACT;

    virtual IRenderEnt* addRenderEnt(RenderEntDesc& ent) X_ABSTRACT;
    virtual void freeRenderEnt(IRenderEnt* pEnt) X_ABSTRACT;
    virtual void updateRenderEnt(IRenderEnt* pEnt, const Transformf& trans, bool force = false) X_ABSTRACT;
    virtual bool setBonesMatrix(IRenderEnt* pEnt, const Matrix44f* pMats, size_t num) X_ABSTRACT;

    virtual IRenderLight* addRenderLight(RenderLightDesc& ent) X_ABSTRACT;

    virtual fx::IEmitter* addEmmiter(EmitterDesc& emit) X_ABSTRACT;
};

X_NAMESPACE_END
