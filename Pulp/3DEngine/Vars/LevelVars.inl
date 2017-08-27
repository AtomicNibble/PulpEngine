

X_NAMESPACE_BEGIN(level)

X_INLINE int32_t LevelVars::usePortals(void) const
{
	return usePortals_;
}


X_INLINE int32_t LevelVars::drawAreaBounds(void) const
{
	return drawAreaBounds_;
}


X_INLINE int32_t LevelVars::drawPortals(void) const
{
	return drawPortals_;
}


X_INLINE int32_t LevelVars::drawArea(void) const
{
	return drawArea_;
}


X_INLINE int32_t LevelVars::drawCurrentAreaOnly(void) const
{
	return drawCurrentAreaOnly_;
}


X_INLINE int32_t LevelVars::drawStats(void) const
{
	return drawStats_;
}


X_INLINE int32_t LevelVars::drawModelBounds(void) const
{
	return drawModelBounds_;
}


X_INLINE int32_t LevelVars::drawModelBones(void) const
{
	return drawModelBones_;
}

X_INLINE int32_t LevelVars::drawModelBoneNames(void) const
{
	return drawModelBoneNames_;
}

X_INLINE int32_t LevelVars::drawPortalStacks(void) const
{
	return drawPortalStacks_;
}


X_INLINE int32_t LevelVars::detachCam(void) const
{
	return detachCam_;
}

X_INLINE float LevelVars::boneNameSize(void) const
{
	return boneNameSize_;
}



X_INLINE int32_t LevelVars::cullEnts(void) const
{
	return cullEnts_;
}

X_INLINE Colorf LevelVars::boneColor(void) const
{
	return boneCol_;
}

X_INLINE Colorf LevelVars::boneNameColor(void) const
{
	return boneNameCol_;
}

X_INLINE Vec3f LevelVars::boneNameOffset(void) const
{
	return boneNameOffset_;
}



X_NAMESPACE_END