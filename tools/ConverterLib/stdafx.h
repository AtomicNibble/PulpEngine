#pragma once

#include <EngineCommon.h>



#ifdef X_LIB
#define CONVERTERLIB_EXPORT 
#else
#ifdef CONVERTER_LIB_EXPORT 
#define CONVERTERLIB_EXPORT X_EXPORT
#else
#define CONVERTERLIB_EXPORT X_IMPORT
#endif // !CONVERTERLIB_EXPORT 
#endif // X_LIB


