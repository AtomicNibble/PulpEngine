




X_INLINE XWinding& XWinding::operator = (const XWinding& winding)
{
	int i;

	if (!EnsureAlloced(winding.numPoints)) {
		numPoints = 0;
		return *this;
	}
	for (i = 0; i < winding.numPoints; i++) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

X_INLINE const Vec3f& XWinding::operator[](const int idx) const
{
	X_ASSERT(idx < numPoints && idx >= 0, "index out of range")(idx, getNumPoints());
	return p[idx];
}

X_INLINE Vec3f&	XWinding::operator[](const int idx)
{
	X_ASSERT(idx < numPoints && idx >= 0,"index out of range")(idx,getNumPoints());
	return p[idx];
}


// add a point to the end of the winding point array
X_INLINE XWinding&XWinding::operator+=(const Vec3f& v)
{
	addPoint(v);
	return *this;
}

X_INLINE void XWinding::addPoint(const Vec3f& v)
{
	if (!EnsureAlloced(numPoints + 1, true)) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}


X_INLINE int XWinding::getNumPoints(void) const
{
	return numPoints;
}

X_INLINE int XWinding::getAllocatedSize(void) const
{
	return allocedSize;
}

