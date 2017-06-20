#pragma once

#ifndef X_LVL_SETTINGS_H_
#define X_LVL_SETTINGS_H_

X_NAMESPACE_BEGIN(lvl)

X_DECLARE_ENUM(ShadowOpt)(NONE, MERGE_SURFACES, CULL_OCCLUDED, CLIP_OCCLUDERS, CLIP_SILS, SIL_OPTIMIZE);

struct Settings
{
	Settings() {
		noPatches = false;
		noTJunc = false;
		nomerge = false;
		noFlood = false;
		noOptimize = true;

		noClipSides = false;
		noLightCarve = false;

		shadowOptLevel = ShadowOpt::NONE;
	}

	bool noPatches;
	bool noTJunc;
	bool nomerge;
	bool noFlood;
	bool noOptimize;

	bool noClipSides;		// don't cut sides by solid leafs, use the entire thing
	bool noLightCarve;		// extra triangle subdivision by light frustums

	ShadowOpt::Enum	shadowOptLevel;
};

extern Settings gSettings;

X_NAMESPACE_END



#endif // !X_LVL_SETTINGS_H_