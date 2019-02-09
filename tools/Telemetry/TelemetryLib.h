#pragma once

#ifndef X_TELEMETRY_LIB_H_
#define X_TELEMETRY_LIB_H_


#ifndef TELEMETRYLIB_EXPORT
#ifdef X_LIB
#define TELEMETRYLIB_EXPORT
#else
#ifdef TELEMETRY_LIB_EXPORT
#define TELEMETRYLIB_EXPORT X_EXPORT
#else
#define TELEMETRYLIB_EXPORT X_IMPORT
#endif // !TELEMETRY_LIB_EXPORT
#endif // X_LIB
#endif // !TELEMETRYLIB_EXPORT


#include "Telemetry.h"

#endif // !X_TELEMETRY_LIB_H_