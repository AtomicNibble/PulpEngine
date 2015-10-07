#include "stdafx.h"
#include "X3DEngine.h"

#include <ITimer.h>
#include <IMaterial.h>

#include <IRender.h>
#include <IRenderAux.h>

#include <IConsole.h>

#include <IGui.h>

#include "ModelLoader.h"
#include "MaterialManager.h"

X_NAMESPACE_BEGIN(engine)

ICore* X3DEngine::pCore_ = nullptr;

core::ITimer* X3DEngine::pTimer_ = nullptr;
core::IFileSys* X3DEngine::pFileSys_ = nullptr;
core::IConsole* X3DEngine::pConsole_ = nullptr;
render::IRender* X3DEngine::pRender_ = nullptr;

// 3d
engine::XMaterialManager* X3DEngine::pMaterialManager_ = nullptr;

gui::XGuiManager* X3DEngine::pGuiManger_ = nullptr;


//------------------------------------------
texture::ITexture* pTex = nullptr;
texture::ITexture* pTex1 = nullptr;
texture::ITexture* pTexSky = nullptr;

gui::IGui* gui = nullptr;

// Commands

void Command_Map(core::IConsoleCmdArgs* Cmd)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->p3DEngine);

	if (Cmd->GetArgCount() != 2)
	{
		X_WARNING("3DEngine", "map <mapname>");
		return;
	}

	const char* mapName = Cmd->GetArg(1);

	X3DEngine* p3DEngine = reinterpret_cast<X3DEngine*>(gEnv->p3DEngine);
	p3DEngine->LoadMap(mapName);
}

void Command_DevMap(core::IConsoleCmdArgs* Cmd)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->p3DEngine);

	if (Cmd->GetArgCount() != 2)
	{
		X_WARNING("3DEngine", "devmap <mapname>");
		return;
	}

	const char* mapName = Cmd->GetArg(1);

	X3DEngine* p3DEngine = reinterpret_cast<X3DEngine*>(gEnv->p3DEngine);
	p3DEngine->LoadDevMap(mapName);
}

// ~Commands

bool X3DEngine::Init()
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pTimer);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);
	X_ASSERT_NOT_NULL(gEnv->pConsole);
	X_ASSERT_NOT_NULL(gEnv->pRender);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	pCore_ = gEnv->pCore;
	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
	pConsole_ = gEnv->pConsole;
	pRender_ = gEnv->pRender;

	// register some file types.
	gEnv->pHotReload->addfileType(this, "level");
	gEnv->pHotReload->addfileType(this, "map");

	pMaterialManager_ = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager");
	pMaterialManager_->Init();
	pGuiManger_ = &guisMan_;

	RegisterCmds();

	guisMan_.Init();

	level::Level::Init();

//	level_.Load("boxmap");
//	level_.Load("portal_test");
	level_.Load("entity_test");
	return true;
}

void X3DEngine::ShutDown()
{
	X_LOG0("3DEngine", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, "level");
	gEnv->pHotReload->addfileType(nullptr, "map");

	guisMan_.Shutdown();

	if (pMaterialManager_) {
		pMaterialManager_->ShutDown();
		X_DELETE(pMaterialManager_, g_3dEngineArena);
	}

	level::Level::ShutDown();
}

int X3DEngine::release(void)
{
	X_DELETE(this,g_3dEngineArena);
	return 0;
}


void X3DEngine::OnFrameBegin(void)
{
	X_PROFILE_BEGIN("3DFrameBegin", core::ProfileSubSys::ENGINE3D);

	level_.update();
	if (level_.canRender()) {
		level_.render();
	}


	// draw me some gui baby
	if (gui) {
		pRender_->setGUIShader();
		gui->Redraw();
	}
}

void X3DEngine::Update(void)
{
	X_PROFILE_BEGIN("3DUpdate", core::ProfileSubSys::ENGINE3D);


}

bool X3DEngine::OnFileChange(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	// do nothing for now.


	return true;
}

// =======================================

void X3DEngine::RegisterCmds(void)
{
	ADD_COMMAND("map", Command_Map, core::VarFlag::SYSTEM, "Loads a map");
	ADD_COMMAND("devmap", Command_DevMap, core::VarFlag::SYSTEM, "Loads a map in developer mode");


}


void X3DEngine::LoadMap(const char* mapName)
{
	X_ASSERT_NOT_NULL(mapName);

	level_.Load(mapName);
}

void X3DEngine::LoadDevMap(const char* mapName)
{
	X_ASSERT_NOT_NULL(mapName);

	// this should not really duplicate anything.
	// set some vars then just load the map normaly tbh.

	LoadMap(mapName);
}


X_NAMESPACE_END
