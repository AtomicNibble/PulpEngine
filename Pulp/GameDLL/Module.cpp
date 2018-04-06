#include "stdafx.h"
#include <ModuleExports.h>

#include <IGame.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

#include "Game.h"

X_USING_NAMESPACE;

GameArena* g_gameArena = nullptr;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace
{
    core::MallocFreeAllocator g_gameAlloc;
}

//////////////////////////////////////////////////////////////////////////
class XEngineModule_Game : public IEngineModule
{
    X_ENGINE_INTERFACE_SIMPLE(IEngineModule);

    X_ENGINE_GENERATE_SINGLETONCLASS(XEngineModule_Game, "Engine_Game",
        0xaa1b4442, 0x7cf8, 0x4992, 0xa8, 0x7e, 0x8c, 0xe5, 0x0, 0x18, 0xe7, 0xf);

    //////////////////////////////////////////////////////////////////////////
    virtual const char* GetName(void) X_OVERRIDE
    {
        return "Game";
    };

    //////////////////////////////////////////////////////////////////////////
    virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);
        X_UNUSED(initParams);

        ICore* pCore = env.pCore;

        LinkModule(pCore, "GameDLL");

        g_gameArena = X_NEW(GameArena, gEnv->pArena, "GameArena")(&g_gameAlloc, "GameArena");

        if (!gEnv->pCore->IntializeLoadedConverterModule(X_ENGINE_OUTPUT_PREFIX "ModelLib", "Engine_ModelLib")) {
            X_ERROR("GameDLL", "Failed to init ModelLib");
            return false;
        }

        auto* pGame = X_NEW(game::XGame, g_gameArena, "XGame")(pCore);

        env.pGame = pGame;
        return true;
    }

    virtual bool ShutDown(void) X_OVERRIDE
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pArena);

        X_DELETE_AND_NULL(g_gameArena, gEnv->pArena);

        return true;
    }
};

X_ENGINE_REGISTER_CLASS(XEngineModule_Game);

XEngineModule_Game::XEngineModule_Game(){};

XEngineModule_Game::~XEngineModule_Game(){};
