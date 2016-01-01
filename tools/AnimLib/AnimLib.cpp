#include "stdafx.h"

#include <IAnimation.h>
#include <IEngineModule.h>

#include <ModuleExports.h>
#include <Extension\XExtensionMacros.h>

#include "XAnimLib.h"


namespace 
{

	core::MallocFreeAllocator g_AnimLibAlloc;


} // namespace

AnimLibrena* g_AnimLibArena = nullptr;

extern "C" DLL_EXPORT X_NAMESPACE_NAME::anim::IAnimLib* CreateInterface(ICore *pCore)
{
	LinkModule(pCore, "AnimLib");

	g_AnimLibArena = X_NEW(AnimLibrena, gEnv->pArena, "AnimLibArena")(&g_AnimLibAlloc, "AnimLibArena");

	return X_NEW(anim::XAnimLib, g_AnimLibArena, "AnimLib")();
}

extern "C" DLL_EXPORT void FreeInterface(X_NAMESPACE_NAME::anim::IAnimLib* pAnimLib)
{
	X_DELETE(pAnimLib, g_AnimLibArena);
	X_DELETE(g_AnimLibArena, gEnv->pArena);
}