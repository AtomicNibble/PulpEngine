#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IRender.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

// #include "../Common/XRender.h"
#include "Dx10Render.h"

X_USING_NAMESPACE;


#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
//	core::SimpleMemoryTracking,
//	core::ExtendedMemoryTracking,
	core::FullMemoryTracking,
	core::SimpleMemoryTagging
> RendererArena;

typedef core::MemoryArena<
	core::GrowingBlockAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
> TextureArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_RenderAlloc;
	core::GrowingBlockAllocator g_TextureDataAlloc;
}

core::MemoryArenaBase* g_rendererArena = nullptr;
core::MemoryArenaBase* g_textureDataArena = nullptr;


extern "C" DLL_EXPORT render::IRender* CreateRender(ICore *pCore)
{
	LinkModule(pCore, "Render");

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);
//	X_ASSERT_NOT_NULL(gEnv->pMalloc);


	g_rendererArena = X_NEW_ALIGNED(RendererArena, gEnv->pArena, "RendererArena", 8)(&g_RenderAlloc, "RendererArena");
	g_textureDataArena = X_NEW(TextureArena, gEnv->pArena, "TextureArena")(&g_TextureDataAlloc, "TextureArena");

	render::IRender* pRender = &render::g_Dx11D3D;

	return pRender;
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderDx10");
	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE{ return "RenderDx10"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		render::IRender* pRender = nullptr;

		pRender = CreateRender(pCore);

		if (!pRender) {
			return false;
		}

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

X_ENGINE_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
{
};

XEngineModule_Render::~XEngineModule_Render()
{
};

