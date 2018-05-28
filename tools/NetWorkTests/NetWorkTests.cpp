#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Console.h>
#include <String\HumanSize.h>
#include <Time\StopWatch.h>

#include <Hashing\Fnva1Hash.h>

#define _LAUNCHER
#include <ModuleExports.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")
X_LINK_ENGINE_LIB("Network")

X_FORCE_LINK_FACTORY("XEngineModule_Network")
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
    >
    ServerTestArena;

core::MemoryArenaBase* g_arena = nullptr;

namespace
{
    static const net::Port SERVER_PORT = 60000;

    void run(core::Console& Console, net::IPeer* pPeer, bool isServer)
    {
        if (isServer) {
            X_LOG0("ServerTest", "Starting as server");

            net::SocketDescriptor sd(SERVER_PORT);
            auto res = pPeer->init(16, sd);
            if (res != net::StartupResult::Started) {
                return;
            }

            pPeer->setPassword(net::PasswordStr("goat"));
            pPeer->setMaximumIncomingConnections(16);
        }
        else {
            X_LOG0("ServerTest", "Starting as client");

            net::SocketDescriptor sd;
            auto res = pPeer->init(1, sd);

            if (res != net::StartupResult::Started) {
                return;
            }

            // connect to server.
            auto connectRes = pPeer->connect(net::IPStr("127.0.0.1"), SERVER_PORT, net::PasswordStr("goat"));
            if (connectRes != net::ConnectionAttemptResult::Started) {
                return;
            }
        }

        X_LOG0("ServerTest", "Waiting for packets..");

        uint8_t testData[64];
        core::zero_object(testData);

        while (1) {
            pPeer->runUpdate();

            net::Packet* pPacket = nullptr;
            for (pPacket = pPeer->receive(); pPacket; pPeer->freePacket(pPacket), pPacket = pPeer->receive()) {
                X_LOG0("ServerTest", "Recived packet: bitLength: %" PRIu32, pPacket->bitLength);
            }

            // sleep, as other thread will handle incoming requests and buffer then for us.
            core::Thread::sleep(50);

            char key = Console.readKey();
            if (key == 'X') {
                break;
            }
        }

        pPeer->shutdown(core::TimeVal::fromMS(500), net::OrderingChannel::Default);
    }

    void ClientServerSelector(core::Console& Console)
    {
        net::INet* pNet = gEnv->pNet;
        net::IPeer* pPeer = pNet->createPeer();

        bool isServer = true;

        X_LOG0("ServerTest", "Press enter for server mode or c+enter for client");
        char key = Console.readKeyBlocking();
        if (key == 'C' || key == 'c') {
            isServer = false;
        }

        if (isServer) {
            Console.setTitle(X_ENGINE_NAME_W L" - Server");
            Console.moveTo(3000, 10);
        }
        else {
            Console.setTitle(X_ENGINE_NAME_W L" - Client");
            Console.moveTo(3000, 800);
        }

        run(Console, pPeer, isServer);

        pNet->deletePeer(pPeer);
    }

} // namespace

const char* googleTestResTostr(int nRes)
{
    if (nRes == 0) {
        return "SUCCESS";
    }
    return "ERROR";
}

X_DECLARE_ENUM(Mode)
(
    UnitTests,
    ClientTest,
    EchoServer);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hPrevInstance);
    X_UNUSED(nCmdShow);

    using namespace core::Hash::Literals;

    {
        core::MallocFreeAllocator allocator;
        ServerTestArena arena(&allocator, "NetworkTestArena");
        g_arena = &arena;

        EngineApp engine;

        if (engine.Init(hInstance, lpCmdLine)) 
        {
            auto& console = *gEnv->pConsoleWnd;
            console.redirectSTD();

            const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");

            Mode::Enum mode = Mode::UnitTests;

            if (pMode) {
                core::StackString<96, char> strLower(pMode);
                strLower.toLower();

                switch (core::Hash::Fnv1aHash(strLower.c_str(), strLower.length())) {
                    case "ut"_fnv1a:
                    case "unit"_fnv1a:
                    case "unittests"_fnv1a:
                        mode = Mode::UnitTests;
                        break;
                    case "client"_fnv1a:
                        mode = Mode::ClientTest;
                        break;
                    case "echo"_fnv1a:
                        mode = Mode::EchoServer;
                        break;
                    default:
                        X_ERROR("Network", "Unknown mode: \"%s\"", strLower.c_str());
                        return -1;
                }
            }

            X_LOG0("NetworkTests", "Mode: \"%s\"", Mode::ToString(mode));


            if (mode == Mode::UnitTests) {
                ::testing::GTEST_FLAG(filter) = "*ConnectToIdleHostFail*";

                X_LOG0("TESTS", "Running unit tests...");
                testing::InitGoogleTest(&__argc, __wargv);

                int nRes = RUN_ALL_TESTS();

                X_LOG0("TESTS", "Tests Complete result: %s", googleTestResTostr(nRes));
            }
            else if (mode == Mode::ClientTest) {
                // start instance as server or client.
                // please talk to me... :X
                ClientServerSelector(console);
            }
            else {
                X_ASSERT_UNREACHABLE();
            }

            console.pressToContinue();
        }

        engine.ShutDown();
    }

    return 0;
}
