#include "stdafx.h"
#include "CBufferManager.h"

#include "CBuffer.h"

#include <IFrameData.h>
#include <CmdBucket.h>


X_NAMESPACE_BEGIN(engine)

namespace 
{

	template <typename T>
	using Unqualified = std::remove_cv_t<std::remove_reference_t<T>>;

} // namespace


CBufferManager::RefCountedCBuf::RefCountedCBuf(render::shader::XCBuffer* pCBuf, render::ConstantBufferHandle handle) :
	pCBuf(pCBuf),
	handle(handle)
{
}



CBufferManager::CBufferManager(core::MemoryArenaBase* arena, render::IRender* pRender) :
	pRender_(pRender),
	perFrameCbs_(arena),
	frameIdx_(0)
{
	perFrameCbs_.setGranularity(4);
}

CBufferManager::~CBufferManager()
{

}

void CBufferManager::shutDown(void)
{
	if (perFrameCbs_.isNotEmpty())
	{
		for (auto& cbRef : perFrameCbs_)
		{
			// pCBuf is not valid here
			X_WARNING("CBufMan", "Dangling device cbuffer refs: %" PRIi32, cbRef.getRefCount());

			pRender_->destoryConstBuffer(cbRef.handle);
		}

		perFrameCbs_.clear();
	}
}

bool CBufferManager::update(core::FrameData& frame, bool othro)
{
	using render::shader::ParamType;

	dirtyFlags_.Clear();

	// time
	setTime(frame.timeInfo.startTimeReal);
	setFrameTime(core::ITimer::Timer::GAME, frame.timeInfo.deltas[core::ITimer::Timer::GAME]);
	setFrameTime(core::ITimer::Timer::UI, frame.timeInfo.deltas[core::ITimer::Timer::UI]);

	// view
	setViewPort(frame.view.viewport);

	if (!othro)
	{
		setMatrixes(frame.view.viewMatrix, frame.view.projMatrix,
			frame.view.viewProjMatrix, frame.view.viewProjInvMatrix);
	}
	else
	{
		setMatrixes(frame.view.viewMatrixOrtho, frame.view.projMatrixOrtho,
			frame.view.viewProjMatrixOrth, frame.view.viewProjMatrixOrth.inverted());
	}

	++frameIdx_;

	return dirtyFlags_.IsAnySet();
}

void CBufferManager::updatePerFrameCBs(render::CommandBucket<uint32_t>& bucket, render::Commands::Nop* pNop)
{
	for (auto& cbRef : perFrameCbs_)
	{
		auto& cb = *cbRef.pCBuf;
		
		if (autoUpdateBuffer(cb))
		{
			// dam fucker needs updateing.
			auto* pCBufUpdate = bucket.appendCommand<render::Commands::CopyConstantBufferData>(pNop, cb.getBindSize());

			char* pAuxData = render::CommandPacket::getAuxiliaryMemory(pCBufUpdate);
			std::memcpy(pAuxData, cb.getCpuData().data(), cb.getBindSize());

			pCBufUpdate->constantBuffer = cbRef.handle;
			pCBufUpdate->pData = pAuxData; 
			pCBufUpdate->size = cb.getBindSize();
		}
	}
}

void CBufferManager::setMatrixes(const Matrix44f& view, const Matrix44f& proj)
{
	setMatrixes(view, proj, 
		view * proj, (view * proj).inverted());
}

void CBufferManager::setMatrixes(const Matrix44f& view, const Matrix44f& proj, 
	const Matrix44f& viewProj, const Matrix44f& viewProjInv)
{
	using render::shader::ParamType;

	if (view_ != view) {
		view_ = view;
		inView_ = view.inverted();

		dirtyFlags_.Set(ParamType::PF_viewMatrix);
		dirtyFlags_.Set(ParamType::PF_cameraToWorldMatrix);
	}
	if (proj_ != proj) {
		proj_ = proj;

		dirtyFlags_.Set(ParamType::PF_projectionMatrix);
	}

	if (viewProj_ != viewProj) {
		viewProj_ = viewProj;

		dirtyFlags_.Set(ParamType::PF_worldToScreenMatrix);
	}

	if (viewProjInv_ != viewProjInv) {
		viewProjInv_ = viewProjInv;

		dirtyFlags_.Set(ParamType::PF_screenToWorldMatrix);
	}
}

bool CBufferManager::autoUpdateBuffer(render::shader::XCBuffer& cbuf)
{
	// if this cbuffer was updated last frame we know what was chnaged since then.
	// so we can skip updating potentially anything.
	const auto cbufVersion = cbuf.getCpuDataVersion();

	// if the version is same and the cbuf only changes everyframe nothing will have changed.
	if (cbufVersion == frameIdx_ && cbuf.getUpdateFreg() == render::shader::UpdateFreq::FRAME)
	{
	//	return false;
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
	//	return false;
	}

	auto& cpuData = cbuf.getCpuData();
	for (int32_t i = 0; i < cbuf.getParamCount(); i++)
	{
		const auto& p = cbuf[i];
		
		// we can skip the copy if not changed.
	//	if (!changed.IsSet(type)) {
	//		continue;
	//	}

		setParamValue(p, cpuData.begin(), cpuData.end());
	}

	return true;
}

bool CBufferManager::autoUpdateBuffer(const render::shader::XCBuffer& cbuf, uint8_t* pDataDst, size_t dstLen)
{
	X_ASSERT(dstLen >= static_cast<size_t>(cbuf.getBindSize()), "Dest buffer is too small")(dstLen, cbuf.getBindSize());

	// skip if this cbuffer don't have any per frame param
	if (!cbuf.containsUpdateFreqs(render::shader::UpdateFreq::FRAME)) {
		return false;
	}

	uint8_t* pEnd = pDataDst + dstLen;

	for (int32_t i = 0; i < cbuf.getParamCount(); i++)
	{
		const auto& p = cbuf[i];
		
		setParamValue(p, pDataDst, pEnd);
	}

	return true;
}


render::ConstantBufferHandle CBufferManager::createCBuffer(const render::shader::XCBuffer& cbuf)
{
	X_ASSERT(cbuf.getHash() != 0, "Hash is not calculated")();
	
	// we only share cb's that are composed of just per frame params.
	if (cbuf.containsOnlyFreq(render::shader::UpdateFreq::FRAME)) 
	{
		// we can share this..
		auto it = std::find_if(perFrameCbs_.begin(), perFrameCbs_.end(), [&cbuf](const RefCountedCBuf& oth) -> bool {
			return cbuf.getHash() == oth.pCBuf->getHash() && oth.pCBuf->isEqual(cbuf);
		});

		if (it != perFrameCbs_.end())
		{
			it->addReference();
			return it->handle;
		}

		if (perFrameCbs_.size() >= MAX_PER_FRAME_CBUF)
		{
			X_WARNING("Cbuf", "Per frame cbuffer counts exceeded limit of %" PRIuS " current: %" PRIuS, MAX_PER_FRAME_CBUF, perFrameCbs_.size());
		}

		auto handle = pRender_->createConstBuffer(cbuf, render::BufUsage::PER_FRAME);

		perFrameCbs_.emplace_back(const_cast<std::add_pointer_t<Unqualified<decltype(cbuf)>>>(&cbuf), handle);

		return handle;
	}

	auto handle = pRender_->createConstBuffer(cbuf, render::BufUsage::PER_FRAME);

	return handle;
}

void CBufferManager::destoryConstBuffer(const render::shader::XCBuffer& cbuf, render::ConstantBufferHandle handle)
{
	if (cbuf.containsOnlyFreq(render::shader::UpdateFreq::FRAME))
	{
		auto it = std::find_if(perFrameCbs_.begin(), perFrameCbs_.end(), [&cbuf](const RefCountedCBuf& oth) -> bool {
			return cbuf.getHash() == oth.pCBuf->getHash() && oth.pCBuf->isEqual(cbuf);
		});

		if (it == perFrameCbs_.end()) {
			X_ASSERT_UNREACHABLE();
		}

		X_ASSERT(it->handle == handle, "Destory CB called with a handle that was not created from the provided CB")();

		if (it->removeReference() == 0)
		{
			pRender_->destoryConstBuffer(it->handle);
			perFrameCbs_.erase(it);
		}

		return;
	}

	pRender_->destoryConstBuffer(handle);
}

X_INLINE void CBufferManager::setParamValue(const render::shader::XShaderParam& param, uint8_t* pBegin, uint8_t* pEnd)
{
	using render::shader::ParamType;

	static_assert(ParamType::FLAGS_COUNT == 15, "ParamType count changed, check if this code needs updating");

	uint8_t* pDst = pBegin + param.getBindOffset();

	const ptrdiff_t space = pEnd - pDst;

	X_ASSERT(space >= param.getNumVecs() * 16, "")(space, param.getBindOffset(), param.getNumVecs());

	switch (param.getType())
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



X_NAMESPACE_END
