#pragma once


#ifndef _X_RENDER_SYS_I_H_
#define _X_RENDER_SYS_I_H_


X_NAMESPACE_BEGIN(engine)

X_DECLARE_ENUM(PrimContext)(CONSOLE, GUI, PROFILE);

class IPrimativeContext;

struct I3DEngine
{
	virtual ~I3DEngine(){};

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool Init(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;

	virtual void OnFrameBegin(void) X_ABSTRACT;

	virtual void Update(void) X_ABSTRACT;


	// each enum has a instance, and you don't own the pointer.
	virtual IPrimativeContext* getPrimContext(PrimContext::Enum user) X_ABSTRACT;

};


X_NAMESPACE_END

#endif // !_X_RENDER_SYS_I_H_
