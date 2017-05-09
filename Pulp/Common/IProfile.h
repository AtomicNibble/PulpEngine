#pragma once

#ifndef X_PROFILER_I_H_
#define X_PROFILER_I_H_

struct ICore;

X_NAMESPACE_BEGIN(core)

namespace profiler
{


class XProfileData;

struct IProfileReg
{
	virtual ~IProfileReg(){};

	virtual void AddStartupProfileData(XProfileData* pData) X_ABSTRACT;
	virtual void AddProfileData(XProfileData* pData) X_ABSTRACT;
};

} // namespace profielr

X_NAMESPACE_END

#endif // !X_PROFILER_I_H_
