#pragma once

#include "Types.h"
#include "Compiler.h"
#include "Configuration.h"
#include "PlatformWin32.h"


// just going to always make this a lib.
#define TELEMETRY_COMLIB_EXPORT

#ifndef TELEMETRY_COMLIB_EXPORT

#ifdef TELEM_LIB
#define TELEMETRY_COMLIB_EXPORT
#else
#ifdef TELEMETRY_COM_LIB_EXPORT
#define TELEMETRY_COMLIB_EXPORT TELEM_EXPORT
#else
#define TELEMETRY_COMLIB_EXPORT TELEM_IMPORT
#endif // !TELEMETRY_COM_LIB_EXPORT
#endif // TELEM_LIB

#endif // !TELEMETRY_COMLIB_EXPORT
