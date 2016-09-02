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
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Img, "Engine_ImgLib");

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


		X_DELETE_AND_NULL(pCon, gEnv->pArena);
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
