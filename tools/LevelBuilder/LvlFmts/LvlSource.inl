
X_NAMESPACE_BEGIN(level)

X_INLINE int32_t LvlSource::findFloatPlane(const Planef& plane)
{
    return planes_.FindPlane(plane, PLANE_NORMAL_EPSILON, PLANE_DIST_EPSILON);
}

X_INLINE const LvlSource::LvlEntsArr& LvlSource::getEntsArr(void) const
{
    return entities_;
}

X_INLINE LvlSource::LvlEntsArr& LvlSource::getEntsArr(void)
{
    return entities_;
}

X_INLINE const AABB& LvlSource::getBounds(void) const
{
    return mapBounds_;
}

X_INLINE const SourceStats& LvlSource::getStats(void) const
{
    return stats_;
}

X_NAMESPACE_END