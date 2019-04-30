#pragma once

#include <EngineCommon.h>

#include "dia2.h"

#if X_64
X_LINK_LIB("amd64/diaguids.lib");
#else
X_LINK_LIB("diaguids.lib");
#endif


#ifndef TELEMETRY_SYMLIB_EXPORT

#ifdef TELEM_LIB
#define TELEMETRY_SYMLIB_EXPORT
#else
#ifdef TELEMETRY_SYM_LIB_EXPORT
#define TELEMETRY_SYMLIB_EXPORT X_EXPORT
#else
#define TELEMETRY_SYMLIB_EXPORT X_IMPORT
#endif // !TELEMETRY_SYM_LIB_EXPORT
#endif // TELEM_LIB

#endif // !TELEMETRY_SYMLIB_EXPORT
