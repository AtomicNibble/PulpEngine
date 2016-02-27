#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Pipe.h>

#define _LAUNCHER
#include <ModuleExports.h>


HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")

#endif // !X_LIB


#include "proto\assetdb.pb.h"

#if X_DEBUG
X_LINK_LIB("libprotobufd")
#else
X_LINK_LIB("libprotobuf")
#endif // !X_DEBUG

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
> ConverterArena;

core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	g_hInstance = hInstance;

	{
		EngineApp engine;

		core::Console Console(L"Potato - Converter");
		Console.RedirectSTD();
		Console.SetSize(60, 40, 2000);
		Console.MoveTo(10, 10);

		core::MallocFreeAllocator allocator;
		ConverterArena arena(&allocator, "ConverterArena");
		g_arena = &arena;

		bool res = false;

		if (engine.Init(lpCmdLine, Console))
		{
			core::IPC::Pipe pipe;

			if (pipe.open(R"(\\.\pipe\\Potato_AssetServer)",
				core::IPC::Pipe::OpenMode::READ | core::IPC::Pipe::OpenMode::WRITE |
				core::IPC::Pipe::OpenMode::SHARE
				))
			{
				GOOGLE_PROTOBUF_VERIFY_VERSION;

				// connect to me baby.
				ULONG serverId;
				ULONG serverSessionId;

				pipe.getServerProcessId(&serverId);
				pipe.getServerSessionId(&serverSessionId);

				std::string msgStr;

				ProtoBuf::AssetDB::Asset asset;
				asset.set_id(4);
				asset.set_name("test_model");
				asset.set_type(ProtoBuf::AssetDB::Asset_AssetType_MODEL);
				asset.SerializePartialToString(&msgStr);

				pipe.write(msgStr.data(), msgStr.size());

				pipe.flush();

			}
			
		}

		// shut down the slut.
		google::protobuf::ShutdownProtobufLibrary();

		engine.ShutDown();
	}

	return 0;
}

