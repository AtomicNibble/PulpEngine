#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Console.h>
#include <INetwork.h>

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");

#endif // !X_LIB


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
> ServerTestArena;

core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	X_UNUSED(hPrevInstance);
	X_UNUSED(nCmdShow);

	{

		core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - Server Test Client");
		Console.RedirectSTD();
		Console.SetSize(60, 40, 2000);
		Console.MoveTo(10, 10);

		core::MallocFreeAllocator allocator;
		ServerTestArena arena(&allocator, "ServerTestArena");
		g_arena = &arena;

		EngineApp engine;

		if (engine.Init(hInstance, lpCmdLine, Console))
		{
			net::INet* pNet = gEnv->pNet;
			net::IPeer* pServer = pNet->createPeer();

			net::SocketDescriptor sd(60000);
			auto res = pServer->init(16, &sd, 1);
			if (res != net::StartupResult::Error)
			{
				pServer->setMaximumIncomingConnections(16);

				net::Packet* pPacket = nullptr;
				for (pPacket = pServer->receive(); pPacket; pServer->freePacket(pPacket), pPacket = pServer->receive())
				{

					X_LOG0("ServerTest", "Recived packet: bitLength: %" PRIu32, pPacket->bitLength);

				}

				pNet->deletePeer(pServer);
			}

		}

		Console.PressToContinue();
		engine.ShutDown();
	}

	return 0;
}

