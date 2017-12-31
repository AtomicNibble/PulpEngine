#pragma once

#include <EngineCommon.h>

#include <Extension\FactoryRegNode.h>


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
X_LINK_LIB("engine_WeaponLib")
X_LINK_LIB("engine_FontLib")

X_FORCE_LINK_FACTORY("XConverterLib_Model");
X_FORCE_LINK_FACTORY("XConverterLib_Anim");
X_FORCE_LINK_FACTORY("XConverterLib_Img");
X_FORCE_LINK_FACTORY("XConverterLib_Material");
X_FORCE_LINK_FACTORY("XConverterLib_Weapon");
X_FORCE_LINK_FACTORY("XConverterLib_Font");


#endif // !X_LIB
