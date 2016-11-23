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
	namespace shader
	{
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


class CBufferManager
{
	typedef core::ReferenceCountedInstance<render::shader::XCBuffer*> RefCountedCBuf;

	struct hash
	{
		size_t operator()(const RefCountedCBuf& cbh) const;
	};

	struct equal_to
	{
		bool operator()(const RefCountedCBuf& lhs, const RefCountedCBuf& rhs) const;
	};

	typedef core::HashMap<
		RefCountedCBuf,
		render::ConstantBufferHandle,
		hash,
		equal_to
	> ConstBufferCacheMap;

public:
	CBufferManager(core::MemoryArenaBase* arena, render::IRender* pRender);
	~CBufferManager();

	void update(core::FrameData& frame);

	void autoFillBuffer(render::shader::XCBuffer& cbuf);

	// create a device cbuffer.
	render::ConstantBufferHandle createCBuffer(render::shader::XCBuffer& cbuf);
	void destoryConstBuffer(render::shader::XCBuffer& cbuf, render::ConstantBufferHandle handle);

private:
	X_INLINE void setTime(core::TimeVal time);
	X_INLINE void setFrameTime(core::ITimer::Timer::Enum timer, core::TimeVal time);
	X_INLINE void setViewPort(const XViewPort& viewport);

private:
	render::IRender* pRender_;
	ConstBufferCacheMap cbMap_;

	int32_t frameIdx_;

private:
	render::shader::ParamTypeFlags dirtyFlags_;

	float time_;
	float frameTime_[core::ITimer::Timer::ENUM_COUNT];

	Vec4f screenSize_;

	Matrix44f view_;
	Matrix44f inView_;
	Matrix44f proj_;

	Matrix44f viewProj_;
	Matrix44f viewProjInv_;
};


X_INLINE void CBufferManager::setTime(core::TimeVal time)
{
	const float newTime = time.GetMilliSeconds();

	if (math<float>::abs(newTime - time_) >= EPSILON_VALUEf)
	{
		time_ = newTime;

		dirtyFlags_.Set(render::shader::ParamType::PF_Time);
	}
}

X_INLINE void CBufferManager::setFrameTime(core::ITimer::Timer::Enum timer, core::TimeVal frameTime)
{
	const float newTime = frameTime.GetMilliSeconds();

	if (math<float>::abs(newTime - frameTime_[timer]) >= EPSILON_VALUEf)
	{
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
