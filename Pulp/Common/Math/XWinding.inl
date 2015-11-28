




X_INLINE XWinding& XWinding::operator = (const XWinding& winding)
{
	int i;

	if (!EnsureAlloced(winding.numPoints_)) {
		numPoints_ = 0;
		return *this;
	}
	for (i = 0; i < winding.numPoints_; i++) {
		pPoints_[i] = winding.pPoints_[i];
	}
	numPoints_ = winding.numPoints_;
	return *this;
}

X_INLINE const Vec5f& XWinding::operator[](const size_t idx) const
{
	X_ASSERT(static_cast<int32_t>(idx) < numPoints_ && idx >= 0, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}

X_INLINE Vec5f&	XWinding::operator[](const size_t idx)
{
	X_ASSERT(static_cast<int32_t>(idx) < numPoints_ && idx >= 0, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}


// add a point to the end of the winding point array
X_INLINE XWinding&XWinding::operator+=(const Vec5f& v)
{
	addPoint(v);
	return *this;
}

X_INLINE XWinding&XWinding::operator+=(const Vec3f& v)
{
	addPoint(v);
	return *this;
}

X_INLINE void XWinding::addPoint(const Vec5f& v)
{
	if (!EnsureAlloced(numPoints_ + 1, true)) {
		return;
	}
	pPoints_[numPoints_] = v;
	numPoints_++;
}

X_INLINE void XWinding::addPoint(const Vec3f& v)
{
	if (!EnsureAlloced(numPoints_ + 1, true)) {
		return;
	}
	pPoints_[numPoints_] = v;
	numPoints_++;
}


X_INLINE size_t XWinding::getNumPoints(void) const
{
	return numPoints_;
}

X_INLINE size_t XWinding::getAllocatedSize(void) const
{
	return allocedSize_;
}



X_INLINE bool XWinding::EnsureAlloced(size_t n, bool keep)
{
	int32_t num = safe_static_cast<int32_t, size_t>(n);
	if (num > allocedSize_) {
		return ReAllocate(num, keep);
	}
	return true;
}

X_INLINE bool XWinding::ReAllocate(int32_t n, bool keep)
{
	Vec5f* oldP;

	oldP = pPoints_;
	
	n = core::bitUtil::RoundUpToMultiple(n, 4);

	pPoints_ = X_NEW_ARRAY(Vec5f, n, gEnv->pArena, "WindingRealoc");

	if (oldP) {
		if (keep) {
			memcpy(pPoints_, oldP, numPoints_ * sizeof(pPoints_[0]));
		}
		X_DELETE_ARRAY(oldP, gEnv->pArena);
	}
	allocedSize_ = n;
	return true;
}