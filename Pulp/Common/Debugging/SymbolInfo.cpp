#include "EngineCommon.h"

#include "SymbolInfo.h"

X_NAMESPACE_BEGIN(core)

SymbolInfo::SymbolInfo(const char* const function, const char* const filename, unsigned int line) :
    function_(function),
    filename_(filename),
    line_(line)
{
}

X_NAMESPACE_END