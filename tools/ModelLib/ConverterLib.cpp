#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "XModelLib.h"

namespace
{

	core::MallocFreeAllocator g_ModelLibAlloc;

} // namespace

ModelLibrena* g_ModelLibArena = nullptr;



class XConverterLib_Model : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Model, "Engine_ModelLib",
	0x20a3b762, 0x30f5, 0x41d9, 0xb4, 0xad, 0xcb, 0x40, 0x2b, 0x2, 0xbd, 0x1b);



	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Model";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_ModelLibArena = X_NEW(ModelLibrena, gEnv->pArena, "ModelLibrena")(&g_ModelLibAlloc, "ModelLibrena");

		return X_NEW(model::XModelLib, g_ModelLibArena, "ModelLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		X_DELETE_AND_NULL(pCon, g_ModelLibArena);
		X_DELETE_AND_NULL(g_ModelLibArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Model);


XConverterLib_Model::XConverterLib_Model()
{
}

XConverterLib_Model::~XConverterLib_Model()
{
}
