#include "stdafx.h"
#include <ModuleExports.h>

#include <IGame.h>
#include <IEngineModule.h>

#include <Extension\XExtensionMacros.h>

#include "Game.h"

X_USING_NAMESPACE;

GameArena* g_gameArena;

// the allocator dose not check for leaks so it
// dose not need to go out of scope.
namespace {
	core::MallocFreeAllocator g_gameAlloc;
}



game::IGame* CreateGame(ICore* pCore)
{
	LinkModule(pCore, "GameDLL");

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);


	g_gameArena = X_NEW(GameArena, gEnv->pArena, "GameArena")(&g_gameAlloc, "GameArena");


	return X_NEW(game::XGame,g_gameArena,"XGame")(pCore);
}


//////////////////////////////////////////////////////////////////////////
class XEngineModule_Game : public IEngineModule
{
	X_GOAT_GENERATE_SINGLETONCLASS(XEngineModule_Game, "Engine_Game");
	//////////////////////////////////////////////////////////////////////////
	virtual const char *GetName() X_OVERRIDE{ return "Game"; };

	//////////////////////////////////////////////////////////////////////////
	virtual bool Initialize(SCoreGlobals& env, const SCoreInitParams& initParams) X_OVERRIDE
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_UNUSED(initParams);

		ICore* pCore = env.pCore;
		game::IGame* pGame;

		pGame = CreateGame(pCore);

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

X_GOAT_REGISTER_CLASS(XEngineModule_Game);

XEngineModule_Game::XEngineModule_Game()
{
};

XEngineModule_Game::~XEngineModule_Game()
{
};

