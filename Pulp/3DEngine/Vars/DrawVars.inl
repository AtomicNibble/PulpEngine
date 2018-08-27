

X_NAMESPACE_BEGIN(engine)

X_INLINE int32_t DrawVars::drawFontDebug(void) const
{
    return drawFontDebug_;
}

X_INLINE int32_t DrawVars::usePortals(void) const
{
    return usePortals_;
}

X_INLINE int32_t DrawVars::drawAreaBounds(void) const
{
    return drawAreaBounds_;
}

X_INLINE int32_t DrawVars::drawPortals(void) const
{
    return drawPortals_;
}

X_INLINE int32_t DrawVars::drawArea(void) const
{
    return drawArea_;
}

X_INLINE int32_t DrawVars::drawCurrentAreaOnly(void) const
{
    return drawCurrentAreaOnly_;
}

X_INLINE int32_t DrawVars::drawStats(void) const
{
    return drawStats_;
}

X_INLINE int32_t DrawVars::drawLightDebug(void) const
{
    return drawLightDebug_;
}

X_INLINE int32_t DrawVars::drawModelBounds(void) const
{
    return drawModelBounds_;
}

X_INLINE int32_t DrawVars::drawModelBones(void) const
{
    return drawModelBones_;
}

X_INLINE int32_t DrawVars::drawModelBoneNames(void) const
{
    return drawModelBoneNames_;
}

X_INLINE int32_t DrawVars::drawPortalStacks(void) const
{
    return drawPortalStacks_;
}

X_INLINE int32_t DrawVars::drawDepth(void) const
{
    return drawDepth_;
}

X_INLINE int32_t DrawVars::drawBuffer2D(void) const
{
    return drawBuffer2D_;
}

X_INLINE int32_t DrawVars::drawBuffer3D(void) const
{
    return drawBuffer3D_;
}

X_INLINE Colorf DrawVars::clearCol2D(void) const
{
    return clearCol2D_;
}

X_INLINE Colorf DrawVars::clearCol3D(void) const
{
    return clearCol3D_;
}

X_INLINE int32_t DrawVars::detachCam(void) const
{
    return detachCam_;
}

X_INLINE float DrawVars::boneNameSize(void) const
{
    return boneNameSize_;
}

X_INLINE int32_t DrawVars::cullEnts(void) const
{
    return cullEnts_;
}

X_INLINE Colorf DrawVars::boneColor(void) const
{
    return boneCol_;
}

X_INLINE Colorf DrawVars::boneNameColor(void) const
{
    return boneNameCol_;
}

X_INLINE Vec3f DrawVars::boneNameOffset(void) const
{
    return boneNameOffset_;
}

X_NAMESPACE_END