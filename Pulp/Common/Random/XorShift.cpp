#include "EngineCommon.h"
#include "XorShift.h"

X_NAMESPACE_BEGIN(core)

namespace random
{
	XorShift::XorShift() :
		state_(
			123456789, 362436069, 521288629, 88675123
		)
	{

	}

	XorShift::XorShift(const Vec4i& seed) :
		state_(seed)
	{

	}

	void XorShift::setSeed(const Vec4i& seed)
	{
		state_ = seed;
	}
	
}


X_NAMESPACE_END