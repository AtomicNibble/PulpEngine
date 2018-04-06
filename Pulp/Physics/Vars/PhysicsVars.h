#pragma once

#include <Util\Delegate.h>
#include "Stepper.h"

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(physics)

class PhysXVars
{
public:
    typedef core::Delegate<void(bool)> DebugDrawEnabledDel;

    typedef char StrBuf[128];

public:
    PhysXVars();
    ~PhysXVars() = default;

    void RegisterVars(void);
    void SetScene(physx::PxScene* pScene);
    void ClearScene(void);
    void SetDebugDrawChangedDel(DebugDrawEnabledDel del);

    const char* getDllOverrideStr(StrBuf& buf) const;
    uint32_t scratchBufferSize(void) const;
    X_INLINE bool enableAllocTracking(void) const;
    X_INLINE StepperType::Enum getStepperType(void) const;

    X_INLINE bool isPVDEnabled(void) const;
    X_INLINE int32_t getPVDPort(void) const;
    X_INLINE int32_t getPVDTimeoutMS(void) const;
    X_INLINE int32_t getPVDFlags(void) const;
    const char* getPVDIp(StrBuf& buf) const;

    X_INLINE int32_t DebugDrawEnabled(void) const;
    X_INLINE int32_t DebugDrawCullEnabled(void) const;
    X_INLINE int32_t UnifiedHeightFieldsEnabled(void) const;

    void SetDebugDrawEnabled(bool enable);
    void SetGravityVecValue(const Vec3f& gravity);
    void SetAllScalesToValue(float32_t val);

private:
    void Var_OnDebugDrawChange(core::ICVar* pVar);
    void Var_OnScaleChanged(core::ICVar* pVar);
    void Var_OnDebugUseCullChange(core::ICVar* pVar);
    void Var_OnStepperStyleChange(core::ICVar* pVar);

private:
    physx::PxScene* pScene_;

    core::ICVar* pVarDllOverride_;
    core::ICVar* pVarScratchBufSize_;
    core::ICVar* pVarStepperType_;
    core::ICVar* pVarDebugDraw_;

    int32_t scratchBufferDefaultSize_;
    int32_t trackAllocations_;
    StepperType::Enum stepperType_;

    int32_t pvdEnable_;
    int32_t pvdPort_;
    int32_t pvdTineoutMS_;
    int32_t pvdFlags_;
    core::ICVar* pVarPvdIp_;

    DebugDrawEnabledDel debugDrawChangedDel_;
    int32_t debugDraw_;
    int32_t debugDrawUseCullBox_;
    int32_t unifiedHeightFields_;
    Vec3f gravityVec_;

    const char* scaleVarNames_[physx::PxVisualizationParameter::eNUM_VALUES];
    core::ICVar* scaleVars_[physx::PxVisualizationParameter::eNUM_VALUES];
};

X_NAMESPACE_END

#include "PhysicsVars.inl"