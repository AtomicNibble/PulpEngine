#include "StdAfx.h"

#include <ModuleExports.h>

#include "Core.h"

core::MemoryArenaBase* g_coreArena = nullptr;

#if !defined(X_LIB)
BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	X_UNUSED(hModule);
	X_UNUSED(lpReserved);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
#endif // !X_LIB


//////////////////////////////////////////////////////////////////////////
struct XSystemEventListner_Core : public ICoreEventListener
{
public:
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE
	{
		X_UNUSED(wparam);
		X_UNUSED(lparam);
		switch (event)
		{
			case CoreEvent::LEVEL_UNLOAD:
				break;

			case CoreEvent::LEVEL_LOAD_START:
			case CoreEvent::LEVEL_LOAD_END:
			{
				break;
			}

			case CoreEvent::LEVEL_POST_UNLOAD:
			{

				break;
			}
		}
	}
};

static XSystemEventListner_Core g_core_event_listener_system;

extern "C"
{
	IPCORE_API ICore* CreateCoreInterface(const SCoreInitParams &startupParams)
	{
		X_ASSERT_NOT_NULL(startupParams.pCoreArena);

		XCore* pCore = NULL;

		g_coreArena = startupParams.pCoreArena;

		if (!g_coreArena) {
			return nullptr;
		}

		pCore = X_NEW_ALIGNED(XCore, startupParams.pCoreArena, "XCore", 16);

		LinkModule(pCore, "Core");

#if defined(X_LIB)
		if (pCore)
		{
			IPotatoFactoryRegistryImpl* pGoatFactoryImpl =
				static_cast<IPotatoFactoryRegistryImpl*>(pCore->GetFactoryRegistry());
			pGoatFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
		}
#endif


		if (!pCore->Init(startupParams))
		{
			// wait till any async events either fail or succeded i don't care.
			// this is just to remove races on shutdown logic.
			(void)pCore->InitAsyncWait();

			if (gEnv && gEnv->pLog) {
				X_ERROR("Core", "Failed to init core");
			}

			X_DELETE(pCore, startupParams.pCoreArena);
			return nullptr;
		}

		pCore->GetCoreEventDispatcher()->RegisterListener(&g_core_event_listener_system);

		return pCore;
	}

}
