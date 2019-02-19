#pragma once

#include <Util/UniquePointer.h>

#if !defined(TELEM_SRV_EXPORT)

#if !defined(X_LIB)
#define TELEM_SRV_EXPORT X_IMPORT
#else
#define TELEM_SRV_EXPORT
#endif

#endif


X_NAMESPACE_BEGIN(telemetry)

struct ITelemServer
{
    virtual ~ITelemServer() = default;

    virtual bool loadApps() X_ABSTRACT;
    virtual bool listen() X_ABSTRACT;
};

TELEM_SRV_EXPORT core::UniquePointer<ITelemServer> createServer(core::MemoryArenaBase* arena);


X_NAMESPACE_END