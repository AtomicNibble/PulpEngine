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

	virtual void onTickPreRender(float dtime) X_ABSTRACT;
	virtual void onTickPostRender(float dtime) X_ABSTRACT;
	virtual void render(void) X_ABSTRACT; // render stuff like debug shapes.

};


X_NAMESPACE_END

#endif // !_X_PHYSICS_I_H_