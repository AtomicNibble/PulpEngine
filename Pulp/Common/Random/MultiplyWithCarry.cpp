#include "EngineCommon.h"
#include "MultiplyWithCarry.h"

X_NAMESPACE_BEGIN(core)

namespace random
{

	uint32_t mwc_z = 0x159a55e5;
	uint32_t mwc_w = 0x1f123bb5;

	const float MWC_ONE_BY_MAX_UINT32 = 1 / 0xffffffff;

}


X_NAMESPACE_END