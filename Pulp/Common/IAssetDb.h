#pragma once



X_NAMESPACE_BEGIN(assetDb)


static const char ASSET_NAME_SLASH = '/';


X_DECLARE_ENUM(AssetType)(
	MODEL, 
	ANIM,
	MATERIAL, 
	IMG,
	WEAPON, // do we want diffent weapon types, or will the asset contain the sub type?
	TURRET,
	// will allow for data driven lights, that can then be placed in editor rather than having fixed light types.
	LIGHT,
	FX,
	// cam shake
	RUMBLE,

	SHELLSHOCK,
	// pre defined collection of models and settings to make up a char.
	CHARACTER,
	// beep beep, be a long time before this type of asset gets implemented...
	VEHICLE,
	// these will be pre defined camera paths
	// that can be played.
	// option to loop, hude hud, trigger note tracks..
	// basically a anim for the camera
	CAMERA
);

X_NAMESPACE_END