#include "stdafx.h"
#include "X3DEngine.h"

#include <ITimer.h>
#include <IMaterial.h>

#include <IRender.h>
#include <IRenderAux.h>

#include <IConsole.h>

#include "ModelLoader.h"
#include "MaterialManager.h"

X_NAMESPACE_BEGIN(engine)

ICore* X3DEngine::pCore = nullptr;

core::ITimer* X3DEngine::pTimer = nullptr;
core::IFileSys* X3DEngine::pFileSys = nullptr;
core::IConsole* X3DEngine::pConsole = nullptr;
render::IRender* X3DEngine::pRender = nullptr;

// 3d
engine::XMaterialManager* X3DEngine::pMaterialManager = nullptr;

texture::ITexture* pTex;
texture::ITexture* pTex1;

bool X3DEngine::Init()
{
	pCore = gEnv->pCore;

	pTimer = gEnv->pTimer;
	pFileSys = gEnv->pFileSys;
	pConsole = gEnv->pConsole;

	pRender = gEnv->pRender;

	pMaterialManager = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager");
	pMaterialManager->Init();


	// load a lvl lol.
//	map.LoadFromFile("killzone"); // mmmmm
	map.LoadFromFile("box"); // mmmmm

		pTex = pRender->LoadTexture("core_assets/Textures/berlin_floors_rock_tile2_c.dds",
			texture::TextureFlags::DONT_STREAM);
		pTex1 = pRender->LoadTexture("core_assets/Textures/model_test.dds",
			texture::TextureFlags::DONT_STREAM);


	LoadModel();
	return false;
}

void X3DEngine::ShutDown()
{
	X_LOG0("3DEngine", "Shutting Down");



	if (pMaterialManager) {
		pMaterialManager->ShutDown();
		X_DELETE(pMaterialManager, g_3dEngineArena);
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

	pRender->SetTexture(pTex1->getTexID());

	if (pMesh)
		pMesh->render();
	
	pRender->SetTexture(pTex->getTexID());

	map.render();
}

void X3DEngine::Update(void)
{
	X_PROFILE_BEGIN("3DUpdate", core::ProfileSubSys::ENGINE3D);


}

void X3DEngine::LoadModel(void)
{
	model::ModelLoader loader;

	if (loader.LoadModel(model, "core_assets/models/bo2_zom.model"))
	{
		pMesh = gEnv->pRender->createRenderMesh(
			(model::MeshHeader*)&model.getLod(0),
			shader::VertexFormat::P3F_C4B_T2F,
			model.getName()
			);

		pMesh->uploadToGpu();
	}
}




X_NAMESPACE_END
