#include "stdafx.h"
#include "X3DEngine.h"

#include <ITimer.h>
#include <IMaterial.h>

#include <IRender.h>
#include <IRenderAux.h>

#include <IConsole.h>

#include "ModelLoader.h"
#include "MaterialManager.h"
#include "Bsp.h"

X_NAMESPACE_BEGIN(engine)

ICore* X3DEngine::pCore = nullptr;

core::ITimer* X3DEngine::pTimer = nullptr;
core::IFileSys* X3DEngine::pFileSys = nullptr;
core::IConsole* X3DEngine::pConsole = nullptr;
render::IRender* X3DEngine::pRender = nullptr;

// 3d
engine::XMaterialManager* X3DEngine::pMaterialManager = nullptr;



bool X3DEngine::Init()
{
	pCore = gEnv->pCore;

	pTimer = gEnv->pTimer;
	pFileSys = gEnv->pFileSys;
	pConsole = gEnv->pConsole;

	pRender = gEnv->pRender;

	pMaterialManager = X_NEW(engine::XMaterialManager, g_3dEngineArena, "MaterialManager");
	pMaterialManager->Init();


	rotation = Vec3f(0, 90, 75);

	ADD_CVAR_REF("r_x", rotation.x, 0, -9999, 9999, 0, "");
	ADD_CVAR_REF("r_y", rotation.y, 0, -9999, 9999, 0, "");
	ADD_CVAR_REF("r_z", rotation.z, 0, -9999, 9999, 0, "");

	ADD_CVAR_REF("p_x", pos.x, 0, -9999, 9999, 0, "");
	ADD_CVAR_REF("p_y", pos.y, 0, -9999, 9999, 0, "");
	ADD_CVAR_REF("p_z", pos.z, 0, -9999, 9999, 0, "");


	ADD_CVAR_REF("width", width, 100, -9999, 9999, 0, "");
	ADD_CVAR_REF("height", height, 100, -9999, 9999, 0, "");

	// load a lvl lol.
	bsp::Bsp map;

	map.LoadFromFile("box"); // mmmmm

//	const bsp::BSPData& data = map.getData();

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

	XFrustum frustum;

	float fov = (75.0f*PIf / 180.0f);

	static const Vec3f	s_angDefaultCam(toRadians(-10.f), 0, toRadians(17.f));
	static const Vec3f	s_vDefaultCamPos(1, 1, 1);

	frustum.setPosition(s_vDefaultCamPos);
	frustum.setAxis(Matrix33f::createRotation(s_angDefaultCam));
	frustum.SetFrustum(width, height, fov, 0.01f, 0.5f, 1.0);

	render::IRenderAux* pAux = gEnv->pRender->GetIRenderAuxGeo();

	render::XAuxGeomRenderFlags flags = pAux->getRenderFlags();
	flags.SetMode2D3DFlag(render::AuxGeom_Mode2D3D::Mode3D);
	pAux->setRenderFlags(flags);

	pAux->drawFrustum(frustum, Col_Red);
}

void X3DEngine::update(void)
{
	X_PROFILE_BEGIN("3DUpdate", core::ProfileSubSys::ENGINE3D);



}

void X3DEngine::LoadModel(void)
{
	model::ModelLoader loader;
	model::XModel model;

	loader.LoadModel(model, "default.model");


}




X_NAMESPACE_END
