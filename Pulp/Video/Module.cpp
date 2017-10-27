#include "stdafx.h"

#include <ModuleExports.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

X_USING_NAMESPACE;


#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h>

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

> VideoArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_VideoAlloc;
}


core::MemoryArenaBase* g_VideoArena = nullptr;


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Video : public IEngineModule
{
	X_POTATO_INTERFACE_SIMPLE(IEngineModule);

	X_POTATO_GENERATE_SINGLETONCLASS(XEngineModule_Video, "Engine_Video",
		0xe659551d, 0x16dd, 0x4f84, 0xb3, 0xa1, 0x85, 0x99, 0x90, 0xf0, 0x73, 0x85);

	//////////////////////////////////////////////////////////////////////////
	virtual const char* GetName(void) X_OVERRIDE { return "Video"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals &env, const SCoreInitParams &initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_ASSERT_NOT_NULL(gEnv->pConsole);
		X_UNUSED(initParams);

	//	video::IVideo* pVideo = nullptr;
	//
	//	g_VideoArena = X_NEW(VideoArena, gEnv->pArena, "VideoArena")(&g_VideoAlloc, "VideoArena");
	//	pVideo = X_NEW(video::XVideo, g_VideoArena, "VideoSys")(g_VideoArena);

		// env.pVideo = pVideo;
		return true;
	}

	virtual bool ShutDown(void) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		X_DELETE_AND_NULL(g_VideoArena, gEnv->pArena);

		return true;
	}
};



X_POTATO_REGISTER_CLASS(XEngineModule_Video);

XEngineModule_Video::XEngineModule_Video()
{
};

XEngineModule_Video::~XEngineModule_Video()
{
};

