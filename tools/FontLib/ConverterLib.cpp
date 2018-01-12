#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\FontLib.h"

namespace
{

	core::MallocFreeAllocator g_FontLibAlloc;

} // namespace

FontLibArena* g_FontLibArena = nullptr;


class XConverterLib_Font : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Font, "Engine_FontLib",
		0xd20a012, 0xcb88, 0x4531, 0x83, 0x32, 0xb9, 0x99, 0x8c, 0x3e, 0x87, 0x25);

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Font";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_FontLibArena = X_NEW(FontLibArena, gEnv->pArena, "FontLibArena")(&g_FontLibAlloc, "FontLibArena");

		return X_NEW(font::FontLib, g_FontLibArena, "FontLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(pCon, g_FontLibArena);
		X_DELETE_AND_NULL(g_FontLibArena, gEnv->pArena);
		return true;
	}
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Font);


XConverterLib_Font::XConverterLib_Font()
{
}

XConverterLib_Font::~XConverterLib_Font()
{
}
