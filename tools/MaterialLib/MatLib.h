#pragma once

#ifndef X_MATLIB_H_
#define X_MATLIB_H_

#include <Core\Compiler.h>

#ifndef MATLIB_EXPORT
#ifdef X_LIB
#define MATLIB_EXPORT
#else
#ifdef MAT_LIB_EXPORT
#define MATLIB_EXPORT X_EXPORT
#else
#define MATLIB_EXPORT X_IMPORT
#endif // !MAT_LIB_EXPORT
#endif // X_LIB
#endif // !MATLIB_EXPORT

#include "Material.h"
#include "Util\MatUtil.h"
#include "TechDefs\TechDefs.h"
#include "TechDefs\TechSetDef.h"

#endif // !X_MATLIB_H_