#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv

#include "Lib\ImgLib.h"

namespace
{

	core::MallocFreeAllocator g_ImgLibAlloc;

} // namespace

ImgLibArena* g_ImgLibArena = nullptr;


class XConverterLib_Img : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Img, "Engine_ImgLib",
	0xdf24d6ac, 0x121c, 0x4c33, 0x8d, 0x59, 0xee, 0xc9, 0x30, 0xad, 0x36, 0x5f);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Img";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_ImgLibArena = X_NEW(ImgLibArena, gEnv->pArena, "ImgLibArena")(&g_ImgLibAlloc, "ImgLibArena");

		return X_NEW(texture::ImgLib, g_ImgLibArena, "ImgLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		texture::ImgLib* pImgLib = static_cast<texture::ImgLib*>(pCon);

		X_DELETE_AND_NULL(pImgLib, g_ImgLibArena);
		X_DELETE_AND_NULL(g_ImgLibArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Img);


XConverterLib_Img::XConverterLib_Img()
{
}

XConverterLib_Img::~XConverterLib_Img()
{
}
