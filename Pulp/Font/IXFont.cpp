#include "stdafx.h"

#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include "XFont.h"
#include "XNullfont.h"

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;

FontArena* g_fontArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_fontAlloc;
}

X_NAMESPACE_NAME::font::IXFontSys* CreateFontInterface(ICore *pCore)
{
	LinkModule(pCore, "XFont");
 
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);
//	X_ASSERT_NOT_NULL(gEnv->pMalloc);

	// kinky shit.
	g_fontArena = X_NEW(FontArena, gEnv->pArena, "InputArena")(&g_fontAlloc, "InputArena");




	if (gEnv->IsDedicated())
	{
#if defined(X_USE_NULLFONT)
		return X_NEW(font::XFontSysNull,g_fontArena, "FontSysNull")();
#else
		return nullptr;
#endif // !X_USE_NULLFONT
	}

#if defined(X_USE_NULLFONT)
	return X_NEW(font::XFontSysNull,g_fontArena, "FontSysNull")();
#else

	return X_NEW(font::XFont,g_fontArena,"fontSys")(pCore);
#endif // !X_USE_NULLFONT
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Font : public IEngineModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Font, "Engine_Font");

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE { return "Font"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);
		ICore* pCore = env.pCore;
		env.pFont = CreateFontInterface(pCore);
		return env.pFont != 0;
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
