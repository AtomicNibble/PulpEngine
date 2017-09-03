#include "stdafx.h"
#include <ModuleExports.h>

#include <ICore.h>
#include <IEngineModule.h>


#include "X3DEngine.h"

#include <Extension\XExtensionMacros.h>



X_USING_NAMESPACE;

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
> Engine3DArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_3dEngineAlloc;
}

core::MemoryArenaBase* g_3dEngineArena = nullptr;

//////////////////////////////////////////////////////////////////////////
class XEngineModule_3DEngine : public IEngineModule
{
	X_POTATO_INTERFACE_SIMPLE(IEngineModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_3DEngine, "Engine_3DEngine",
	0x99ba3725, 0xbfd1, 0x4950, 0xbc, 0xc4, 0xda, 0xe7, 0xce, 0x71, 0xef, 0xe6);


	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE{ return "3DEngine"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_ASSERT_NOT_NULL(gEnv->pCore);
		X_UNUSED(initParams);

		g_3dEngineArena = X_NEW_ALIGNED(Engine3DArena, gEnv->pArena, "3DEngineArena", 8)(&g_3dEngineAlloc, "3DEngineArena");

		if (!gEnv->pCore->IntializeLoadedConverterModule("Engine_MaterialLib", "Engine_MaterialLib")) {
			X_ERROR("3DEngine", "Failed to init MaterialLib");
			return false;
		}
		if (!gEnv->pCore->IntializeLoadedConverterModule("Engine_ImgLib", "Engine_ImgLib")) {
			X_ERROR("3DEngine", "Failed to init ImgLib");
			return false;
		}
		if (!gEnv->pCore->IntializeLoadedConverterModule("Engine_ModelLib", "Engine_ModelLib")) {
			X_ERROR("3DEngine", "Failed to init ModelLib");
			return false;
		}
		if (!gEnv->pCore->IntializeLoadedConverterModule("Engine_AnimLib", "Engine_AnimLib")) {
			X_ERROR("3DEngine", "Failed to init AnimLib");
			return false;
		}

		auto* pEngine = X_NEW(engine::X3DEngine, g_3dEngineArena, "3DEngine")(g_3dEngineArena);

		env.p3DEngine = pEngine;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_3dEngineArena, gEnv->pArena);

		return true;
	}
};



X_POTATO_REGISTER_CLASS(XEngineModule_3DEngine);

XEngineModule_3DEngine::XEngineModule_3DEngine()
{
};

XEngineModule_3DEngine::~XEngineModule_3DEngine()
{
};

