#include "stdafx.h"
#include "CBufferManager.h"

#include "CBuffer.h"

#include <IFrameData.h>

X_NAMESPACE_BEGIN(engine)



size_t CBufferManager::hash::operator()(const RefCountedCBuf& cbh) const
{
	auto* pCuf = *cbh.instance();
	return static_cast<size_t>(pCuf->getHash()); // drop top 32MSB's in 32bit
}

bool CBufferManager::equal_to::operator()(const RefCountedCBuf& lhs, const RefCountedCBuf& rhs) const
{
	auto* pCuflhs = *lhs.instance();
	auto* pCufrhs = *rhs.instance();

	return pCuflhs->isEqual(*pCufrhs);
}

CBufferManager::CBufferManager(core::MemoryArenaBase* arena, render::IRender* pRender) :
	pRender_(pRender),
	cbMap_(arena, 128),
	frameIdx_(0)
{

}

CBufferManager::~CBufferManager()
{

}

void CBufferManager::update(core::FrameData& frame)
{
	using render::shader::ParamType;

	dirtyFlags_.Clear();

	// time
	setTime(frame.timeInfo.startTimeReal);
	setFrameTime(core::ITimer::Timer::GAME, frame.timeInfo.deltas[core::ITimer::Timer::GAME]);
	setFrameTime(core::ITimer::Timer::UI, frame.timeInfo.deltas[core::ITimer::Timer::UI]);

	// view
	setViewPort(frame.view.viewport);

	if (view_ != frame.view.viewMatrix) {
		view_ = frame.view.viewMatrix;
		inView_ = frame.view.viewMatrix.inverted();

		dirtyFlags_.Set(ParamType::PF_viewMatrix);
		dirtyFlags_.Set(ParamType::PF_cameraToWorldMatrix);
	}
	if (proj_ != frame.view.projMatrix) {
		proj_ = frame.view.projMatrix;

		dirtyFlags_.Set(ParamType::PF_projectionMatrix);
	}

	if (viewProj_ != frame.view.viewProjMatrix) {
		viewProj_ = frame.view.viewProjMatrix;

		dirtyFlags_.Set(ParamType::PF_worldToScreenMatrix);
	}

	if (viewProjInv_ != frame.view.viewProjInvMatrix) {
		viewProjInv_ = frame.view.viewProjInvMatrix;

		dirtyFlags_.Set(ParamType::PF_screenToWorldMatrix);
	}

	++frameIdx_;
}

void CBufferManager::autoFillBuffer(render::shader::XCBuffer& cbuf)
{
	// if this cbuffer was updated last frame we know what was chnaged since then.
	// so we can skip updating potentially anything.
	const auto cbufVersion = cbuf.getCpuDataVersion();

	// if the version is same and the cbuf only changes everyframe nothing will have changed.
	if (cbufVersion == frameIdx_ && cbuf.getUpdateFreg() == render::shader::UpdateFreq::FRAME)
	{
		return;
	}

	auto changed = cbuf.getParamFlags();
	if ((cbufVersion + 1) == frameIdx_)
	{
		changed &= dirtyFlags_;
	}

	cbuf.setCpuDataVersion(frameIdx_);

	// nothing to update.
	if (!changed.IsAnySet()) 
	{
		return;
	}

	auto& cpuData = cbuf.getCpuData();
	for (int32_t i = 0; i < cbuf.getParamCount(); i++)
	{
		const auto& p = cbuf[i];
		uint8_t* pDst = &cpuData[p.getBindPoint()];

		using render::shader::ParamType;

		static_assert(ParamType::FLAGS_COUNT == 15, "ParamType count changed, check if this code needs updating");

		const auto type = p.getType();

		// we can skip the copy if not changed.
		if (!changed.IsSet(type)) {
			continue;
		}

		switch (type)
		{
			case ParamType::PF_worldToScreenMatrix:
				std::memcpy(pDst, &viewProj_, sizeof(viewProj_));
				break;
			case ParamType::PF_screenToWorldMatrix:
				std::memcpy(pDst, &viewProjInv_, sizeof(viewProjInv_));
				break;
			case ParamType::PF_worldToCameraMatrix:
				std::memcpy(pDst, &view_, sizeof(view_));
				break;
			case ParamType::PF_cameraToWorldMatrix:
				std::memcpy(pDst, &inView_, sizeof(inView_));
				break;

			case ParamType::PF_Time:
				std::memcpy(pDst, &time_, sizeof(time_));
				break;
			case ParamType::PF_FrameTime:
				std::memcpy(pDst, &frameTime_[core::ITimer::Timer::GAME], sizeof(frameTime_[core::ITimer::Timer::GAME]));
				break;
			case ParamType::PF_FrameTimeUI:
				std::memcpy(pDst, &frameTime_[core::ITimer::Timer::GAME], sizeof(frameTime_[core::ITimer::Timer::GAME]));
				break;
			case ParamType::PF_ScreenSize:
				std::memcpy(pDst, &screenSize_, sizeof(screenSize_));
				break;


			case ParamType::Unknown:
				// need to be manually set.
				break;

			default:
				X_ASSERT_NOT_IMPLEMENTED();
				break;
		}
	}
}

render::ConstantBufferHandle CBufferManager::createCBuffer(render::shader::XCBuffer& cbuf)
{
	// so this manager should be responsible for merging duplicate cbuffers.
	// if a cbuffer is the same in everyway we can share it.
	// 1. reduces memory usage on gpu.
	// 2. reduces amount of updates required.
	// This is the responsibility of the 3dengine so that each render system don't have to implement this logic.
	// we can have a map that checks the camels twice.
	X_ASSERT(cbuf.getHash() != 0, "Hash is not calculated")();
	
	RefCountedCBuf key(&cbuf);

	auto it = cbMap_.find(key);
	if (it != cbMap_.end())
	{
		it->first.addReference();
		return it->second;
	}

	auto handle = pRender_->createConstBuffer(cbuf, render::BufUsage::DYNAMIC);

	// we starting to get into this odd place where alot of things
	// keep pointers to the cbuffer instances.
	// and i don't think ref counting is a good solution in this case.
	// as the life of the cbuffer should be tied to the shader, not who uses it last.
	cbMap_.insert(std::make_pair(key, handle));

	return handle;
}

void CBufferManager::destoryConstBuffer(render::shader::XCBuffer& cbuf, render::ConstantBufferHandle handle)
{
	// we need ref counting in order to know when we can free the device buffer.
	// we should have the ref couting implemented here.
	// so the value needs to contain a ref count.

	// how do we find the cbuffer instnace for the given handle tho?
	// i need a way to get back to the cbuffer pointer that was used to make the cbuffer.
	// should i just require the cbuffer isntance also?

	RefCountedCBuf key(&cbuf);

	auto it = cbMap_.find(key);
	if (it == cbMap_.end())
	{
		X_ASSERT_UNREACHABLE();
	}

	X_ASSERT(it->second == handle, "Destory CB called with a handle that was not created from the provided CB")();

	if (it->first.removeReference() == 0)
	{
		cbMap_.erase(it);
	}
}

X_NAMESPACE_END
