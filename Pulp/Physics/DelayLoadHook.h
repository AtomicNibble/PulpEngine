#pragma once

#include <common\windows\PxWindowsDelayLoadHook.h>

X_NAMESPACE_BEGIN(physics)

class SampleDelayLoadHook : public physx::PxDelayLoadHook
{
public:

	virtual const char* getPhysXCommonDEBUGDllName(void) const X_FINAL;
	virtual const char* getPhysXCommonCHECKEDDllName(void) const X_FINAL;
	virtual const char* getPhysXCommonPROFILEDllName(void) const X_FINAL;
	virtual const char* getPhysXCommonDllName(void) const X_FINAL;
};

extern SampleDelayLoadHook gDelayLoadHook;

X_NAMESPACE_END