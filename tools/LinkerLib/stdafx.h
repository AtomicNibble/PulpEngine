#pragma once

#include <EngineCommon.h>
#include <Extension\FactoryRegNode.h>


#ifdef X_LIB
#define LINKERLIB_EXPORT 
#else
#ifdef LINKER_LIB_EXPORT 
#define LINKERLIB_EXPORT X_EXPORT
#else
#define LINKERLIB_EXPORT X_IMPORT
#endif // LINKERLIB_EXPORT 
#endif // X_LIB
