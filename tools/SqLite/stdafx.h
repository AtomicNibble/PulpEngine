#pragma once

#include <SDKDDKVer.h>

#include <EngineCommon.h>


#ifdef X_LIB
#define SQL_EXPORT
#else
#define SQL_EXPORT X_EXPORT
#endif // X_LIB