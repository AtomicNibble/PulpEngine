#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IRender.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>
#include <Debugging\InvalidParameterHandler.h>


#include "RenderNull.h"

X_USING_NAMESPACE;


#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> RendererArena;


typedef core::MemoryArena<
	core::GrowingBlockAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> TextureArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_RenderAlloc;
	core::GrowingBlockAllocator g_TextureDataAlloc;

}

core::MemoryArenaBase* g_rendererArena = nullptr;
core::MemoryArenaBase* g_textureDataArena = nullptr;


render::IRender* CreateRender(ICore *pCore)
{
	LinkModule(pCore, "RenderNull");

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);


	g_rendererArena = X_NEW(RendererArena, gEnv->pArena, "RendererArena")(&g_RenderAlloc, "RendererArena");
	g_textureDataArena = X_NEW(TextureArena, gEnv->pArena, "TextureArena")(&g_TextureDataAlloc, "TextureArena");

	render::IRender* pRender = &render::g_NullRender;

	return pRender;
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
	X_POTATO_INTERFACE_SIMPLE(IEngineModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderNull",
	0x7072e440, 0x4bf4, 0x4b09, 0xaf, 0xb5, 0xd8, 0x71, 0xeb, 0x3, 0xc8, 0xc3);


	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE{ return "RenderNull"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		render::IRender *pRender = 0;

		pRender = CreateRender(pCore);

		env.pRender = pRender;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE(g_rendererArena, gEnv->pArena);
		X_DELETE(g_textureDataArena, gEnv->pArena);

		return true;
	}
};

X_POTATO_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
{
};

XEngineModule_Render::~XEngineModule_Render()
{
};

