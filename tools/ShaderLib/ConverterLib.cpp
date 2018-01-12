#include "stdafx.h"

#include <IConverterModule.h>

#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


#include "ShaderLib\ShaderLib.h"

namespace
{

	core::MallocFreeAllocator g_ShaderLibAlloc;

} // namespace

ShaderLibArena* g_ShaderLibArena = nullptr;



class XConverterLib_Shader : public IConverterModule
{
	X_ENGINE_INTERFACE_SIMPLE(IConverterModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XConverterLib_Shader, "Engine_ShaderLib",
	0xd5c97196, 0x6f7c, 0x4294, 0xa5, 0x19, 0xfb, 0x11, 0xf1, 0x1f, 0xfb, 0xc3);


	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Shader";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_ShaderLibArena = X_NEW(ShaderLibArena, gEnv->pArena, "ShaderLibArena")(&g_ShaderLibAlloc, "ShaderLibArena");
		
		return X_NEW(render::shader::ShaderLib, g_ShaderLibArena, "ShaderLib")();
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		X_DELETE_AND_NULL(pCon, g_ShaderLibArena);
		X_DELETE_AND_NULL(g_ShaderLibArena, gEnv->pArena);
		return true;
	}
};

X_ENGINE_REGISTER_CLASS(XConverterLib_Shader);


XConverterLib_Shader::XConverterLib_Shader()
{
}

XConverterLib_Shader::~XConverterLib_Shader()
{
}
