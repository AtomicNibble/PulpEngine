

X_INLINE XWinding::XWinding(void)
{
	numPoints = allocedSize = 0;
	p = nullptr;
}

X_INLINE XWinding::XWinding(const int n)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	EnsureAlloced(n);
}

X_INLINE XWinding::XWinding(const Vec3f *verts, const int n)
{
	int i;

	numPoints = allocedSize = 0;
	p = nullptr;
	if (!EnsureAlloced(n)) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < n; i++)
	{
		p[i].asVec3() = verts[i];
		p[i].s = p[i].t = 0.0f;
	}
	numPoints = n;
}

X_INLINE XWinding::XWinding(const Vec3f &normal, const float dist)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane(normal, dist);
}

X_INLINE XWinding::XWinding(const Planef &plane)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane(plane);
}

X_INLINE XWinding::XWinding(const XWinding &winding) 
{
	// must be init before calling EnsureAlloced.
	numPoints = allocedSize = 0;
	p = nullptr;

	int i;
	if (!EnsureAlloced(winding.GetNumPoints())) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < winding.GetNumPoints(); i++) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

X_INLINE XWinding::~XWinding(void)
{
	Clear();
}

// -------------------------------------

X_INLINE XWinding& XWinding::operator=(const XWinding &winding)
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

X_INLINE const Vec5f& XWinding::operator[](const int index) const
{
	return p[index];
}

X_INLINE Vec5f& XWinding::operator[](const int index)
{
	return p[index];
}

// number of points on winding
X_INLINE int XWinding::GetNumPoints(void) const
{
	return numPoints;
}

X_INLINE void XWinding::SetNumPoints(int n)
{
	if (!EnsureAlloced(n, true)) {
		return;
	}
	numPoints = n;
}


X_INLINE bool XWinding::IsTiny(void) const
{
	int		i;
	float	len;
	Vec3f	delta;
	int		edges;

	edges = 0;
	for (i = 0; i < numPoints; i++) {
		delta = p[(i + 1) % numPoints].xyz() - p[i].xyz();
		len = delta.length();
		if (len > EDGE_LENGTH) {
			if (++edges == 3) {
				return false;
			}
		}
	}
	return true;
}


X_INLINE bool XWinding::IsHuge(void) const
{
	int i, j;

	for (i = 0; i < numPoints; i++) {
		for (j = 0; j < 3; j++) {
			if (p[i][j] <= level::MIN_WORLD_COORD || p[i][j] >= level::MAX_WORLD_COORD) {
				return true;
			}
		}
	}
	return false;
}