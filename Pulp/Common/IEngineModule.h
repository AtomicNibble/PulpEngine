#pragma once

#ifndef _X_ENGINEMODULE_I_H_
#define _X_ENGINEMODULE_I_H_

#include <Extension\IEngineUnknown.h>

struct CoreInitParams;

// Base Interface for all engine module extensions
struct IEngineModule : public IEngineUnknown
{
    ENGINE_INTERFACE_DECLARE(IEngineModule, 0xd4c7e5cb, 0xf73e, 0x4fdd, 0x97, 0x62, 0x91, 0x4f, 0xe5, 0x9f, 0x5d, 0xd3);

    // Retrieve name of the extension module.
    virtual const char* GetName(void) X_ABSTRACT;

    // This is called to initialize the new module.
    virtual bool Initialize(SCoreGlobals& env, const CoreInitParams& initParams) X_ABSTRACT;
    virtual bool ShutDown(void) X_ABSTRACT;
};

#endif // !_X_ENGINEMODULE_I_H_
