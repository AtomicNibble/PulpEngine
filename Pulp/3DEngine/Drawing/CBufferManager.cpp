#include "stdafx.h"
#include "CBufferManager.h"

#include "CBuffer.h"

X_NAMESPACE_BEGIN(engine)



size_t CBufferManager::hash::operator()(const render::shader::XCBuffer* cbh) const
{
	return cbh->getHash();
}

bool CBufferManager::equal_to::operator()(const render::shader::XCBuffer* lhs, const render::shader::XCBuffer* rhs) const
{
	return lhs->isEqual(*rhs);
}

CBufferManager::CBufferManager(core::MemoryArenaBase* arena, render::IRender* pRender) :
	pRender_(pRender),
	cbMap_(arena, 128)
{

}

CBufferManager::~CBufferManager()
{

}


void CBufferManager::autoFillBuffer(render::shader::XCBuffer& cbuf)
{
	// set the values for all the auto params.
	using render::shader::ParamType;

	auto& cpuData = cbuf.getCpuData();

	for (int32_t i = 0; i < cbuf.getParamCount(); i++)
	{
		const auto& p = cbuf[i];
		uint8_t* pDst = &cpuData[p.getBindPoint()];

		switch (p.getType())
		{
			case ParamType::PF_worldToScreenMatrix:
				std::memcpy(pDst, &viewProj_, sizeof(viewProj_));
				break;
			case ParamType::PF_worldToCameraMatrix:
				std::memcpy(pDst, &view_, sizeof(view_));
				break;
			case ParamType::PF_cameraToWorldMatrix:
				std::memcpy(pDst, &inView_, sizeof(inView_));
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
	
	auto it = cbMap_.find(&cbuf);
	if (it != cbMap_.end())
	{
		return it->second;
	}

	auto handle = pRender_->createConstBuffer(cbuf, render::BufUsage::DYNAMIC);

	// we starting to get into this odd place where alot of things
	// keep pointers to the cbuffer instances.
	// and i don't think ref counting is a good solution in this case.
	// as the life of the cbuffer should be tied to the shader, not who uses it last.
	cbMap_.insert(std::make_pair(&cbuf, handle));

	return handle;
}


X_NAMESPACE_END
