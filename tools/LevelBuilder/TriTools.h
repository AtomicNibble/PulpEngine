#pragma once


/*

X_INLINE float MapTriArea(const mapTri_t *tri) 
{
	return XWinding::TriangleArea(tri->v[0].pos, tri->v[1].pos, tri->v[2].pos);
}

X_INLINE void PlaneForTri(const mapTri_t *tri, Planef &plane) {
	plane = Planef(tri->v[0].pos, tri->v[1].pos, tri->v[2].pos);
}


XWinding *WindingForTri(const mapTri_t *tri);
mapTri_t* CopyMapTri(const mapTri_t *tri);
mapTri_t* MergeTriLists(mapTri_t *a, mapTri_t *b);
void FreeTriList(mapTri_t *a);

int	CountTriList(const mapTri_t *tri);


mapTri_t* AllocTri(void);
void FreeTri(mapTri_t *tri);


void TriVertsFromOriginal(mapTri_t *tri, const mapTri_t *original);
mapTri_t *WindingToTriList(const XWinding *w, const mapTri_t *originalTri);


*/