#pragma once

#ifndef _X_RENDER_SYS_H_
#define _X_RENDER_SYS_H_

#include "I3DEngine.h"
#include "EngineBase.h"

X_NAMESPACE_BEGIN(engine)


struct X3DEngine : public I3DEngine, public XEngineBase
{

	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual int release(void) X_OVERRIDE;

	virtual void OnFrameBegin(void) X_OVERRIDE;

	virtual void update(void) X_OVERRIDE;

	virtual void LoadModel(void) X_OVERRIDE;



private:
	Vec3f rotation, pos;

	int width;
	int height;
};


X_NAMESPACE_END


#endif // !_X_RENDER_SYS_H_
