#pragma once

#ifndef X_LINKER_LIB_H_
#define X_LINKER_LIB_H_

#include <Core\Compiler.h>

#ifndef LINKERLIB_EXPORT
#ifdef X_LIB
#define LINKERLIB_EXPORT
#else
#ifdef LINKER_LIB_EXPORT
#define LINKERLIB_EXPORT X_EXPORT
#else
#define LINKERLIB_EXPORT X_IMPORT
#endif // !LINKER_LIB_EXPORT
#endif // X_LIB
#endif // !LINKERLIB_EXPORT


#include "Linker.h"
#include "AssetList.h"

#endif // !X_LINKER_LIB_H_