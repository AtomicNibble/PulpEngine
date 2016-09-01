#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


namespace
{

	core::MallocFreeAllocator g_MatLibAlloc;

} // namespace

MatLibrena* g_MatLibArena = nullptr;


class XConverterLib_Material : public IConverterModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Material, "Engine_MateriaLib");

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Material";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_MatLibArena = X_NEW(MatLibrena, gEnv->pArena, "MaterialLibArena")(&g_MatLibAlloc, "MaterialLibArena");

		// nothing yet.
		X_ASSERT_NOT_IMPLEMENTED();
		return nullptr;
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
