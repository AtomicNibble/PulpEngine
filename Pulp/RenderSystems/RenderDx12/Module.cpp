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
	core::SingleThreadPolicy,
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
	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Render, "Engine_RenderDx12");
	
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
		g_textureDataArena = X_NEW(TextureArena, gEnv->pArena, "TextureArena")(&g_TextureDataAlloc, "TextureArena");
		pRender = X_NEW(XRender, g_rendererArena, "XRender")(g_rendererArena);

		if (!pRender) {
			return false;
		}

		if (!pCore->IntializeLoadedConverterModule("Engine_ImgLib", "Engine_ImgLib")) {
			X_ERROR("Render", "Failed to init imgLib");

			X_DELETE(pRender, g_rendererArena);
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

X_POTATO_REGISTER_CLASS(XEngineModule_Render);

XEngineModule_Render::XEngineModule_Render()
{
};

XEngineModule_Render::~XEngineModule_Render()
{
};


X_NAMESPACE_END
