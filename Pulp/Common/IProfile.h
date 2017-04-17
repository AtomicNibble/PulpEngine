#pragma once

#ifndef X_PROFILER_I_H_
#define X_PROFILER_I_H_

struct ICore;

X_NAMESPACE_BEGIN(core)

class XProfileData;

struct IProfileSys
{
	virtual ~IProfileSys(){};

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(ICore* pCore) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;

	virtual void AddProfileData(XProfileData* pData) X_ABSTRACT;

	virtual void OnFrameBegin(void) X_ABSTRACT;
	virtual void OnFrameEnd(void) X_ABSTRACT;

	virtual void Render(void) X_ABSTRACT;

};


X_NAMESPACE_END

#endif // !X_PROFILER_I_H_
