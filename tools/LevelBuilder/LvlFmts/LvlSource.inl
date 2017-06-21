
X_NAMESPACE_BEGIN(lvl)


X_INLINE int32_t LvlSource::findFloatPlane(const Planef& plane)
{
	return planes_.FindPlane(plane, PLANE_NORMAL_EPSILON, PLANE_DIST_EPSILON);
}

X_NAMESPACE_END