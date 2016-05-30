#pragma once


#ifndef _X_PHYSICS_I_H_
#define _X_PHYSICS_I_H_

#include <IConverterModule.h>

X_NAMESPACE_BEGIN(physics)

struct IPhysics
{
	virtual ~IPhysics() {}

	virtual void RegisterVars(void) X_ABSTRACT;
	virtual void RegisterCmds(void) X_ABSTRACT;

	virtual bool Init(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_PHYSICS_I_H_