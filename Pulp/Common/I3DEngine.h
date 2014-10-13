#pragma once


#ifndef _X_RENDER_SYS_I_H_
#define _X_RENDER_SYS_I_H_


X_NAMESPACE_BEGIN(engine)

struct I3DEngine
{
	virtual ~I3DEngine(){};

	virtual bool Init(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual int release(void) X_ABSTRACT;

	virtual void OnFrameBegin(void) X_ABSTRACT;

	virtual void update(void) X_ABSTRACT;



	virtual void LoadModel(void) X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_RENDER_SYS_I_H_
