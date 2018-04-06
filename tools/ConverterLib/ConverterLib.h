#pragma once

#ifndef X_CONVERTER_LIB_H_
#define X_CONVERTER_LIB_H_

#include <Core\Compiler.h>

#ifndef CONVERTERLIB_EXPORT
#ifdef X_LIB
#define CONVERTERLIB_EXPORT
#else
#ifdef CONVERTER_LIB_EXPORT
#define CONVERTERLIB_EXPORT X_EXPORT
#else
#define CONVERTERLIB_EXPORT X_IMPORT
#endif // !CONVERTER_LIB_EXPORT
#endif // X_LIB
#endif // !CONVERTERLIB_EXPORT

#include "Converter.h"

#endif // !X_CONVERTER_LIB_H_