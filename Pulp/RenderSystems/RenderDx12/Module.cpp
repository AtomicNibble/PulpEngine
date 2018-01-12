#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>


#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include "XRender.h"

X_NAMESPACE_BEGIN(render)


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> RendererArena;

typedef core::MemoryArena<
	core::GrowingBlockAllocator,
	core::MultiThreadPolicy<core::CriticalSection>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> TextureArena;


// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_RenderAlloc;
	core::GrowingBlockAllocator g_TextureDataAlloc;
}

core::MemoryArenaBase* g_rendererArena = nullptr;
core::MemoryArenaBase* g_textureDataArena = nullptr;


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Render : public IEngineModule
{
	X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

	X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderDx12",
	0x241a57f0, 0xe743, 0x4aaa, 0x8b, 0xd2, 0x84, 0x4, 0x6b, 0xc3, 0x47, 0xb9);

	
	virtual const char* GetName(void) X_OVERRIDE { return "RenderDx12"; };

	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		IRender* pRender = nullptr;

		LinkModule(pCore, "Render");

		g_rendererArena = X_NEW_ALIGNED(RendererArena, gEnv->pArena, "RendererArena", 8)(&g_RenderAlloc, "RendererArena");
		g_textureDataArena = X_NEW(TextureArena, g_rendererArena, "TextureArena")(&g_TextureDataAlloc, "TextureArena");

		// call these before render construction.
#if 0
		if (!pCore->IntializeLoadedConverterModule("Engine_ImgLib", "Engine_ImgLib")) {
			X_ERROR("Render", "Failed to init imgLib");
			return false;
		}
#endif

		if (!pCore->IntializeLoadedConverterModule("Engine_ShaderLib", "Engine_ShaderLib")) {
			X_ERROR("Render", "Failed to init shaderLib");
			return false;
		}
	
		pRender = X_NEW(XRender, g_rendererArena, "XRender")(g_rendererArena);
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

		X_DELETE(g_textureDataArena, g_rendererArena);
		X_DELETE(g_rendererArena, gEnv->pArena);

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


X_NAMESPACE_END
