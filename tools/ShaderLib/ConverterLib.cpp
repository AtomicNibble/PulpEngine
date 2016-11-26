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
	X_POTATO_GENERATE_SINGLETONCLASS(XConverterLib_Shader, "Engine_ShaderLib");

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

X_POTATO_REGISTER_CLASS(XConverterLib_Shader);


XConverterLib_Shader::XConverterLib_Shader()
{
}

XConverterLib_Shader::~XConverterLib_Shader()
{
}
