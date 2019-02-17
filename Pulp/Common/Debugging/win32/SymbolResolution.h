#pragma once

#ifndef X_SYMBOLRESOLUTION_H_
#define X_SYMBOLRESOLUTION_H_

#include "Debugging/SymbolInfo.h"

X_NAMESPACE_BEGIN(core)


namespace symbolResolution
{
    void Startup(void);
    void Refresh(void);
    void Shutdown(void);

    SymbolInfo ResolveSymbolsForAddress(const void* const address);
} // namespace symbolResolution

X_NAMESPACE_END

#endif
