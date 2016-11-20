#include "stdafx.h"
#include "CBufferManager.h"

#include "CBuffer.h"

X_NAMESPACE_BEGIN(engine)



CBufferManager::CBufferManager()
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


X_NAMESPACE_END
