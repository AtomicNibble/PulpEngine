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



private:
	int32_t usePortals_;
	int32_t drawAreaBounds_;
	int32_t drawPortals_;
	int32_t drawArea_;
	int32_t drawCurrentAreaOnly_;
	int32_t drawStats_;
	int32_t drawModelBounds_;
	int32_t drawModelBones_;
	int32_t drawPortalStacks_;
	int32_t detechCam_;
	int32_t cullEnts_;
};


X_NAMESPACE_END

#include "LevelVars.inl"