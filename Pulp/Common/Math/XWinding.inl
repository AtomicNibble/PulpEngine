




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

X_INLINE const Vec5f& XWinding::operator[](const int idx) const
{
	X_ASSERT(idx < numPoints_ && idx >= 0, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}

X_INLINE Vec5f&	XWinding::operator[](const int idx)
{
	X_ASSERT(idx < numPoints_ && idx >= 0, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}


// add a point to the end of the winding point array
X_INLINE XWinding&XWinding::operator+=(const Vec3f& v)
{
	addPoint(v);
	return *this;
}

X_INLINE void XWinding::addPoint(const Vec3f& v)
{
	if (!EnsureAlloced(numPoints_ + 1, true)) {
		return;
	}
	pPoints_[numPoints_] = v;
	numPoints_++;
}


X_INLINE int XWinding::getNumPoints(void) const
{
	return numPoints_;
}

X_INLINE int XWinding::getAllocatedSize(void) const
{
	return allocedSize_;
}

