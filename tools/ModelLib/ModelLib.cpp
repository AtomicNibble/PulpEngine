#include "stdafx.h"

/*


#include <IAnimation.h>
#include <IEngineModule.h>

#include <ModuleExports.h>
#include <Extension\XExtensionMacros.h>

#include "XModelLib.h"


namespace
{

	core::MallocFreeAllocator g_ModelLibAlloc;


} // namespace

ModelLibrena* g_ModelLibArena = nullptr;

extern "C" DLL_EXPORT X_NAMESPACE_NAME::model::IModelLib* CreateInterface(ICore *pCore)
{
	LinkModule(pCore, "AnimLib");

	g_ModelLibArena = X_NEW(ModelLibrena, gEnv->pArena, "ModelLibrena")(&g_ModelLibAlloc, "ModelLibrena");

	return X_NEW(model::XModelLib, g_ModelLibArena, "ModelLib")();
}

extern "C" DLL_EXPORT void FreeInterface(X_NAMESPACE_NAME::model::IModelLib* pModelLib)
{
	X_DELETE(pModelLib, g_ModelLibArena);
	X_DELETE(g_ModelLibArena, gEnv->pArena);
}

*/