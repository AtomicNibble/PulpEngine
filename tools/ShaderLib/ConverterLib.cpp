#include "stdafx.h"

#include <IConverterModule.h>

#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


namespace
{

	core::MallocFreeAllocator g_ShaderLibAlloc;

} // namespace

ShaderLibArena* g_ShaderLibArena = nullptr;



class XConverterLib_Shader : public IConverterModule
{
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Shader, "Engine_Lib");

	virtual const char* GetName(void) X_OVERRIDE
	{
		return "Shader";
	}

	virtual IConverter* Initialize(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		g_ShaderLibArena = X_NEW(ShaderLibArena, gEnv->pArena, "ShaderLibArena")(&g_ShaderLibAlloc, "ShaderLibArena");

		return nullptr;
	}

	virtual bool ShutDown(IConverter* pCon) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);


		X_DELETE_AND_NULL(pCon, gEnv->pArena);
		X_DELETE_AND_NULL(g_ShaderLibArena, gEnv->pArena);
		return true;
	}
};

X_POTATO_REGISTER_CLASS(XConverterLib_Shader);


XConverterLib_Shader::XConverterLib_Shader()
{
}

XConverterLib_Shader::~XConverterLib_Shader()
{
}
