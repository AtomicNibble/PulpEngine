#include "stdafx.h"
#include "ProfileNull.h"


X_NAMESPACE_BEGIN(core)

namespace profiler
{

	void ProfileNull::AddStartupProfileData(XProfileData* pData)
	{
		X_UNUSED(pData);
	}

	void ProfileNull::AddProfileData(XProfileData* pData)
	{
		X_UNUSED(pData);
	}

} // namespace profiler

X_NAMESPACE_END


