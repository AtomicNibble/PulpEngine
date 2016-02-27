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
		X_LOG0("AssetServer", "Hello :)");

		// alright then shitface, we want to start a pipe for now.

		core::IPC::Pipe pipe;

		if (pipe.create(R"(\\.\pipe\\Potato_AssetServer)",

			core::IPC::Pipe::CreateMode::DUPLEX,
			core::IPC::Pipe::PipeMode::MESSAGE_RW,
			10,
			1024,
			1024,
			core::TimeVal::fromMS(100)
			))
		{
			// connect to me baby.
			if (pipe.connect())
			{
				GOOGLE_PROTOBUF_VERIFY_VERSION;

				ProtoBuf::AssetDB::Asset asset;

		//		asset.ParsePartialFromString();
				// create a thread to service this client.
				char buf[1024];
				size_t bytesRead;

				pipe.read(buf, sizeof(buf), &bytesRead);

				asset.ParsePartialFromString(std::string(buf));

				buf[0] = 0;
			}
		}

		// shut down the slut.
		google::protobuf::ShutdownProtobufLibrary();

		engine.ShutDown();
	}

	return 0;
}

