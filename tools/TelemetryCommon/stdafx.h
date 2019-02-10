#pragma once

#include "Types.h"
#include "Compiler.h"
#include "PlatformWin32.h"
#include "Util.h"



#ifndef TELEMETRY_COMLIB_EXPORT

#ifdef X_LIB
#define TELEMETRY_COMLIB_EXPORT
#else
#ifdef TELEMETRY_COM_LIB_EXPORT
#define TELEMETRY_COMLIB_EXPORT X_EXPORT
#else
#define TELEMETRY_COMLIB_EXPORT X_IMPORT
#endif // !TELEMETRY_COM_LIB_EXPORT
#endif // X_LIB

#endif // !TELEMETRY_COMLIB_EXPORT