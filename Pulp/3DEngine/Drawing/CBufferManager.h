#pragma once

#include <Time\TimeVal.h>

#include <Math\XMatrix44.h>
#include <Math\XViewPort.h>

#include <Containers\HashMap.h>

#include <Hashing\xxHash.h>

#include <Util\ReferenceCounted.h>

#include <IShader.h>
#include <ITimer.h>

X_NAMESPACE_DECLARE(render,
    namespace shader {
        class XCBuffer;
    } // nmaespace shader
);

X_NAMESPACE_BEGIN(engine)

// this is for storing the automated values.
// so the idea is that we can just set the predefined values here and it works out if values have changed.
// if some values have changed that are part of the per frame const buffers it will schedule a update of the cbuf data to the render sys.
// this way we don't have to do redundancy checks in the render sys and we can store the redundancy check in a single place in 3degnine.
// instead of redundancy checks anywhere we update cbuffers that have like frame and batch update freq.
//
//	I'm not sure how i'm going to handle updating of cbuffers that are update per draw.
//	i don't want to have to update values in the manager before everydraw..
//	Or is that actually sane.. since it will just copy the value to destination.
//  I guess as long as it's limited or somthing.. humm..
//
//	I also want to maybe merge const buffers.
//	If a const buffer for two diffrent shaders is the same and is bound is same locations and everything.
//	they can both use the same gpu memory and we only have to update one.
//

X_DISABLE_WARNING(4324) // structure was padded due to alignment specifier

class CBufferManager
{
    struct RefCountedCBuf : public core::ReferenceCounted<>
    {
        RefCountedCBuf(render::shader::XCBuffer* pCBuf, render::ConstantBufferHandle handle);

        render::shader::XCBuffer* pCBuf;
        render::ConstantBufferHandle handle;
    };

    typedef core::Array<RefCountedCBuf> CBufRefArr;

    static const size_t MAX_PER_FRAME_CBUF = 16;

public:
    CBufferManager(core::MemoryArenaBase* arena, render::IRender* pRender);
    ~CBufferManager();

    void shutDown(void);

    bool update(core::FrameData& frame, bool othro);
    void updatePerFrameCBs(render::CommandBucket<uint32_t>& bucket, render::Commands::Nop* pNop);

    void setMatrixes(const Matrix44f& view, const Matrix44f& proj);
    void setMatrixes(const Matrix44f& view, const Matrix44f& proj,
        const Matrix44f& viewProj, const Matrix44f& viewProjInv);

    // returns true if stale.
    bool autoUpdateBuffer(render::shader::XCBuffer& cbuf);
    // if the cbuffer contains any auto update params
    // the values are placed into pDst at the correct bind offsets.
    // no dirty checks are performed
    bool autoUpdateBuffer(const render::shader::XCBuffer& cbuf, uint8_t* pDst, size_t dstLen);

    // create a device cbuffer.
    render::ConstantBufferHandle createCBuffer(const render::shader::XCBuffer& cbuf);
    void destoryConstBuffer(const render::shader::XCBuffer& cbuf, render::ConstantBufferHandle handle);

    void setParamValue(const render::shader::XShaderParam& param, uint8_t* pBegin, uint8_t* pEnd);

private:
    X_INLINE void setTime(core::TimeVal time);
    X_INLINE void setFrameTime(core::ITimer::Timer::Enum timer, core::TimeVal time);
    X_INLINE void setViewPort(const XViewPort& viewport);

private:
    render::IRender* pRender_;
    CBufRefArr perFrameCbs_;

    int32_t frameIdx_;

private:
    render::shader::ParamTypeFlags dirtyFlags_;

    float time_;
    float frameTime_[core::ITimer::Timer::ENUM_COUNT];

    Vec4f screenSize_;

    X_ALIGN16_MATRIX44F(view_);
    X_ALIGN16_MATRIX44F(inView_);
    X_ALIGN16_MATRIX44F(proj_);

    X_ALIGN16_MATRIX44F(viewProj_);
    X_ALIGN16_MATRIX44F(viewProjInv_);
};

X_ENABLE_WARNING(4324)

X_INLINE void CBufferManager::setTime(core::TimeVal time)
{
    const float newTime = time.GetMilliSeconds();

    if (math<float>::abs(newTime - time_) >= EPSILON_VALUEf) {
        time_ = newTime;

        dirtyFlags_.Set(render::shader::ParamType::PF_Time);
    }
}

X_INLINE void CBufferManager::setFrameTime(core::ITimer::Timer::Enum timer, core::TimeVal frameTime)
{
    const float newTime = frameTime.GetMilliSeconds();

    if (math<float>::abs(newTime - frameTime_[timer]) >= EPSILON_VALUEf) {
        frameTime_[timer] = newTime;

        static_assert(core::ITimer::Timer::ENUM_COUNT == 2, "Timer count changed? check this code");

        if (timer == core::ITimer::Timer::Enum::GAME) {
            dirtyFlags_.Set(render::shader::ParamType::PF_FrameTime);
        }
        else if (timer == core::ITimer::Timer::Enum::UI) {
            dirtyFlags_.Set(render::shader::ParamType::PF_FrameTimeUI);
        }
        else {
            X_ASSERT_UNREACHABLE();
        }
    }
}

X_INLINE void CBufferManager::setViewPort(const XViewPort& viewport)
{
    Vec4f screenSize;
    screenSize.x = viewport.getWidthf();
    screenSize.y = viewport.getHeightf();
    screenSize.z = 0.5f / screenSize.x;
    screenSize.w = 0.5f / screenSize.y;

    if (screenSize_ != screenSize) {
        screenSize_ = screenSize;
        dirtyFlags_.Set(render::shader::ParamType::PF_ScreenSize);
    }
}

X_NAMESPACE_END
