#include "stdafx.h"

#include <IConverterModule.h>

#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "XAnimLib.h"


namespace
{

	core::MallocFreeAllocator g_AnimLibAlloc;

} // namespace

AnimLibrena* g_AnimLibArena = nullptr;



class XConverterLib_Anim : public IConverterModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Anim, "Engine_AnimLib");

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Anim";
	}

	virtual bool Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_AnimLibArena = X_NEW(AnimLibrena, gEnv->pArena, "AnimLibArena")(&g_AnimLibAlloc, "AnimLibArena");

		anim::XAnimLib* pAnimLib = X_NEW(anim::XAnimLib, g_AnimLibArena, "AnimLib")();

		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{


		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Anim);


XConverterLib_Anim::XConverterLib_Anim()
{
}

XConverterLib_Anim::~XConverterLib_Anim()
{
}
