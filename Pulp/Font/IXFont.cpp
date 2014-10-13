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

extern "C" X_NAMESPACE_NAME::font::IXFontSys* CreateFontInterface(ICore *pCore)
{
	LinkModule(pCore, "XFont");
 
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);
//	X_ASSERT_NOT_NULL(gEnv->pMalloc);

	// kinky shit.
	g_fontArena = X_NEW_ALIGNED(FontArena, gEnv->pArena, "InputArena", 8)(&g_fontAlloc, "InputArena");




	if (gEnv->IsDedicated())
	{
#if defined(X_USE_NULLFONT)
		return X_NEW_ALIGNED(font::XFontSysNull,g_fontArena, "FontSysNull", 8)();
#else
		return nullptr;
#endif // !X_USE_NULLFONT
	}

#if defined(X_USE_NULLFONT)
	return X_NEW_ALIGNED(font::XFontSysNull,g_fontArena, "FontSysNull", 8)();
#else

	return X_NEW_ALIGNED(font::XFont,g_fontArena,"fontSys",8)(pCore);
#endif // !X_USE_NULLFONT
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Font : public IEngineModule
{
	X_GOAT_GENERATE_SINGLETONCLASS(XEngineModule_Font, "Engine_Font");

	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE { return "Font"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
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

X_GOAT_REGISTER_CLASS(XEngineModule_Font);

XEngineModule_Font::XEngineModule_Font()
{
};

XEngineModule_Font::~XEngineModule_Font()
{

};
