#include "stdafx.h"
#include "DelayLoadHook.h"

X_NAMESPACE_BEGIN(physics)

SampleDelayLoadHook gDelayLoadHook;

const char* SampleDelayLoadHook::getPhysXCommonDEBUGDllName(void) const
{
	return "PhysXCommonDEBUG.dll";
}

const char* SampleDelayLoadHook::getPhysXCommonCHECKEDDllName(void) const
{
	return "PhysXCommonCHECKED.dll";
}

const char* SampleDelayLoadHook::getPhysXCommonPROFILEDllName(void) const
{
	return "PhysXCommonPROFILE.dll";
}

const char* SampleDelayLoadHook::getPhysXCommonDllName(void) const
{
	return "PhysXCommon.dll";
}


X_NAMESPACE_END