#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\FxLib.h"

namespace
{

	core::MallocFreeAllocator g_FxLibAlloc;

} // namespace

FxLibArena* g_FxLibArena = nullptr;


class XConverterLib_Fx : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Fx, "Engine_FxLib",
		0xf2128947, 0xe902, 0x4a02, 0xaa, 0x53, 0xbb, 0x7, 0x8a, 0xb5, 0x65, 0xec);

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Video";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_FxLibArena = X_NEW(FxLibArena, gEnv->pArena, "FxLibArena")(&g_FxLibAlloc, "FxLibArena");

		return X_NEW(engine::fx::FxLib, g_FxLibArena, "FxLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(pCon, g_FxLibArena);
		X_DELETE_AND_NULL(g_FxLibArena, gEnv->pArena);
		return true;
	}
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Fx);


XConverterLib_Fx::XConverterLib_Fx()
{
}

XConverterLib_Fx::~XConverterLib_Fx()
{
}
