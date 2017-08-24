#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(level)


class LevelVars
{
public:
	LevelVars();
	~LevelVars() = default;

	void registerVars(void);

	X_INLINE int32_t usePortals(void) const;
	X_INLINE int32_t drawAreaBounds(void) const;
	X_INLINE int32_t drawPortals(void) const;
	X_INLINE int32_t drawArea(void) const;
	X_INLINE int32_t drawCurrentAreaOnly(void) const;
	X_INLINE int32_t drawStats(void) const;
	X_INLINE int32_t drawModelBounds(void) const;
	X_INLINE int32_t drawModelBones(void) const;
	X_INLINE int32_t drawModelBoneNames(void) const;
	X_INLINE int32_t drawPortalStacks(void) const;
	X_INLINE int32_t detachCam(void) const;
	X_INLINE int32_t cullEnts(void) const;

	X_INLINE Colorf boneColor(void) const;
	X_INLINE Colorf boneNameColor(void) const;


private:
	int32_t usePortals_;
	int32_t drawAreaBounds_;
	int32_t drawPortals_;
	int32_t drawArea_;
	int32_t drawCurrentAreaOnly_;
	int32_t drawStats_;
	int32_t drawModelBounds_;
	int32_t drawModelBones_;
	int32_t drawModelBoneNames_;
	int32_t drawPortalStacks_;
	int32_t detachCam_;
	int32_t cullEnts_;

	Colorf boneCol_;
	Colorf boneNameCol_;
};


X_NAMESPACE_END

#include "LevelVars.inl"