#include "stdafx.h"

#include <IConverterModule.h>

#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "XAnimLib.h"


namespace
{

	core::MallocFreeAllocator g_AnimLibAlloc;

} // namespace

AnimLibArena* g_AnimLibArena = nullptr;



class XConverterLib_Anim : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Anim, "Engine_AnimLib", 
	0xac497e0a, 0x2cbd, 0x441c, 0x95, 0xc1, 0xe5, 0xc6, 0x3e, 0x81, 0xda, 0x48);

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Anim";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_AnimLibArena = X_NEW(AnimLibArena, gEnv->pArena, "AnimLibArena")(&g_AnimLibAlloc, "AnimLibArena");

		return X_NEW(anim::XAnimLib, g_AnimLibArena, "AnimLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		X_DELETE_AND_NULL(pCon, g_AnimLibArena);
		X_DELETE_AND_NULL(g_AnimLibArena, gEnv->pArena);
		return true;
	}
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Anim);


XConverterLib_Anim::XConverterLib_Anim()
{
}

XConverterLib_Anim::~XConverterLib_Anim()
{
}
