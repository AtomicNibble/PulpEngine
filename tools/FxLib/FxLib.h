#pragma once

#include <Core\Compiler.h>

#ifndef FXLIB_EXPORT
#ifdef X_LIB
#define FXLIB_EXPORT
#else
#ifdef FX_LIB_EXPORT
#define FXLIB_EXPORT X_EXPORT
#else
#define FXLIB_EXPORT X_IMPORT
#endif // !FX_LIB_EXPORT
#endif // X_LIB
#endif // !FXLIB_EXPORT

#include "Util\FxUtil.h"