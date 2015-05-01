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

}

void Command_DevMap(core::IConsoleCmdArgs* Cmd)
{

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

	pCore_ = gEnv->pCore;

	pTimer_ = gEnv->pTimer;
	pFileSys_ = gEnv->pFileSys;
	pConsole_ = gEnv->pConsole;

	pRender_ = gEnv->pRender;
	pMaterialManager_ = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager");
	pMaterialManager_->Init();
	pGuiManger_ = &guisMan_;

	RegisterCmds();

	guisMan_.Init();
	return true;
}

void X3DEngine::ShutDown()
{
	X_LOG0("3DEngine", "Shutting Down");

	guisMan_.Shutdown();

	if (pMaterialManager_) {
		pMaterialManager_->ShutDown();
		X_DELETE(pMaterialManager_, g_3dEngineArena);
	}

}

int X3DEngine::release(void)
{
	X_DELETE(this,g_3dEngineArena);
	return 0;
}


void X3DEngine::OnFrameBegin(void)
{
	X_PROFILE_BEGIN("3DFrameBegin", core::ProfileSubSys::ENGINE3D);

//	pRender_->SetTexture(pTex1->getTexID());

//	if (pMesh)
//		pMesh->render();

//	pRender_->SetTexture(pTex->getTexID());


//	pRender_->DefferedBegin();

//	map.render();

//	pRender_->DefferedEnd();
//	pRender_->s

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

// =======================================

void X3DEngine::RegisterCmds(void)
{
	ADD_COMMAND("map", Command_Map, core::VarFlag::SYSTEM, "Loads a map");
	ADD_COMMAND("devmap", Command_DevMap, core::VarFlag::SYSTEM, "Loads a map in developer mode");


}





X_NAMESPACE_END
