#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


#include "Lib\MaterialLib.h"

namespace
{

	core::MallocFreeAllocator g_MatLibAlloc;

} // namespace

MatLibrena* g_MatLibArena = nullptr;


class XConverterLib_Material : public IConverterModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Material, "Engine_MaterialLib");

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Material";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_MatLibArena = X_NEW(MatLibrena, gEnv->pArena, "MaterialLibArena")(&g_MatLibAlloc, "MaterialLibArena");

		return X_NEW(engine::MaterialLib, g_MatLibArena, "IMaterialLib")();

	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		X_DELETE_AND_NULL(pCon, gEnv->pArena);
		X_DELETE_AND_NULL(g_MatLibArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Material);


XConverterLib_Material::XConverterLib_Material()
{
}

XConverterLib_Material::~XConverterLib_Material()
{
}
