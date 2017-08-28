#include "stdafx.h"
#include "DrawVars.h"

#include <IConsole.h>
#include <ILevel.h>

X_NAMESPACE_BEGIN(engine)



DrawVars::DrawVars()
{
	usePortals_ = 1;
	drawAreaBounds_ = 0;
	drawPortals_ = 0;
	drawArea_ = -1;
	drawCurrentAreaOnly_ = 0;
	drawStats_ = 0;
	drawModelBounds_ = 0;
	drawModelBones_ = 0;
	drawPortalStacks_ = 0;
	detachCam_ = 0;
	cullEnts_ = 0;
}

void DrawVars::registerVars(void)
{
	ADD_CVAR_REF("r_drawFontDebug", drawFontDebug_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Draw the font debug view");


	ADD_CVAR_REF("r_usePortals", usePortals_, 1, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Use area portals when rendering the level");

	ADD_CVAR_REF("r_drawAreaBounds", drawAreaBounds_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws bounding box around each level area. 1=visble 2=all 3=visble-fill 4=all-fill");

	ADD_CVAR_REF("r_drawPortals", drawPortals_, 1, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws the inter area portals. 0=off 1=solid 2=solid_dt");

	ADD_CVAR_REF("r_drawArea", drawArea_, -1, -1, level::MAP_MAX_AREAS,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Draws the selected area index. -1 = disable");

	ADD_CVAR_REF("r_drawCurAreaOnly", drawCurrentAreaOnly_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Draws just the current area. 0=off 1=on");

	ADD_CVAR_REF("r_drawStats", drawStats_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Draws frame stats");

	ADD_CVAR_REF("r_drawModelBounds", drawModelBounds_, 0, 0, 4,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draws bounds around models. 1=visible-AABB 2=visible=Sphere 3=all-AABB 4=all-Sphere");

	ADD_CVAR_REF("r_drawModelBones", drawModelBones_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draw model bones. 0=off 1=on");

	ADD_CVAR_REF("r_drawModelBoneNames", drawModelBoneNames_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Draw model bones. 0=off 1=on");

	ADD_CVAR_REF("r_drawPortalStacks", drawPortalStacks_, 0, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED, "Draws portal stacks");

	ADD_CVAR_REF("r_detachCam", detachCam_, 0, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT, "Detaches the camera");

	ADD_CVAR_REF("r_cullEnts", cullEnts_, 0, 0, 2,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Perform visibility culling on entities");

	ADD_CVAR_REF("r_boneNameSize", boneNameSize_, 1.f, 0.01f, 64.f,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Bone name text size");

	// Colors
	ADD_CVAR_REF_COL("r_boneCol", boneCol_, Colorf(0.3f, 0.7f, 0.7f),
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Bone color");

	ADD_CVAR_REF_COL("r_boneNameCol", boneNameCol_, Col_Whitesmoke,
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Bone name color");

	ADD_CVAR_REF_VEC3("r_boneNameOffset", boneNameOffset_, Vec3f(0.f,0.f,1.f),
		core::VarFlag::SYSTEM | core::VarFlag::CHEAT | core::VarFlag::SAVE_IF_CHANGED,
		"Bone name offset");
}



X_NAMESPACE_END