#include "stdafx.h"

#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include "XFontSystem.h"
#include "XNullfont.h"

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;

FontArena* g_fontArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_fontAlloc;
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Font : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Font, "Engine_Font");

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE { return "Font"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		env.pFontSys = nullptr;


		LinkModule(pCore, "XFont");

		// kinky shit.
		g_fontArena = X_NEW(FontArena, gEnv->pArena, "InputArena")(&g_fontAlloc, "InputArena");


		font::IFontSys* pFontSys = nullptr;

		if (gEnv->IsDedicated())
		{
#if defined(X_USE_NULLFONT)
			pFontSys = X_NEW(font::XFontSysNull, g_fontArena, "FontSysNull")();
#else
			// we just say ok.
			return true;
#endif // !X_USE_NULLFONT
		}

#if defined(X_USE_NULLFONT)
		pFontSys = X_NEW(font::XFontSysNull, g_fontArena, "FontSysNull")();
#else

		pFontSys = X_NEW(font::XFontSystem, g_fontArena, "fontSys")(pCore);
#endif // !X_USE_NULLFONT

		if (!pFontSys) {
			return false;
		}

		if (!pFontSys->Init()) {
			X_DELETE(pFontSys, g_fontArena);
			return false;
		}

		env.pFontSys = pFontSys;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_fontArena, gEnv->pArena);

		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_Font);

XEngineModule_Font::XEngineModule_Font()
{
};

XEngineModule_Font::~XEngineModule_Font()
{

};
