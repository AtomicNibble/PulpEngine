#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(physics)

class XPhysics : public IPhysics
{
public:
	XPhysics();
	~XPhysics() X_OVERRIDE;

	// IPhysics
	void RegisterVars(void) X_FINAL;
	void RegisterCmds(void) X_FINAL;

	bool Init(void) X_FINAL;
	void ShutDown(void) X_FINAL;
	void release(void) X_FINAL;
	// ~IPhysics


private:

};


X_NAMESPACE_END