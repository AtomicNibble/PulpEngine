#pragma once

#ifndef _X_ENGINEMODULE_I_H_
#define _X_ENGINEMODULE_I_H_


#include <Extension\IGoatClass.h>

struct SCoreInitParams;

// Base Interface for all engine module extensions
struct IEngineModule : public IGoatClass
{
	// Retrieve name of the extension module.
	virtual const char *GetName() X_ABSTRACT;

	// This is called to initialize the new module.
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_ABSTRACT;
	virtual bool ShutDown(void) X_ABSTRACT;
};


#endif // !_X_ENGINEMODULE_I_H_
