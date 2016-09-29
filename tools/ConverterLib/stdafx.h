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


#ifdef X_LIB

X_LINK_LIB("engine_ModelLib")
X_LINK_LIB("engine_AnimLib")
X_LINK_LIB("engine_ImgLib")
X_LINK_LIB("engine_MaterialLib")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Model@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Anim@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Img@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Material@@0V12@A")

#endif // !X_LIB
