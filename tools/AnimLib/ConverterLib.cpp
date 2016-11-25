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
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Anim, "Engine_AnimLib");

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

X_POTATO_REGISTER_CLASS(XConverterLib_Anim);


XConverterLib_Anim::XConverterLib_Anim()
{
}

XConverterLib_Anim::~XConverterLib_Anim()
{
}
