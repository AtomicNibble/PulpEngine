#pragma once

#ifndef X_TELEMETRY_LIB_H_
#define X_TELEMETRY_LIB_H_

#if defined(WIN32)
#if !defined(X_DLL)
#ifndef TELEM_LIB
#define TELEM_LIB
#endif
#else
#ifndef _USRDLL
#define _USRDLL
#endif
#endif
#endif // WIN32


#ifndef TELEMETRYLIB_EXPORT

#ifdef TELEM_LIB
#define TELEMETRYLIB_EXPORT
#else
#ifdef TELEMETRY_LIB_EXPORT
#define TELEMETRYLIB_EXPORT TELEM_EXPORT
#else
#define TELEMETRYLIB_EXPORT TELEM_IMPORT
#endif // !TELEMETRY_LIB_EXPORT
#endif // TELEM_LIB

#endif // !TELEMETRYLIB_EXPORT


#include "Telemetry.h"

#endif // !X_TELEMETRY_LIB_H_