#include "stdafx.h"

#include "BSPTypes.h"

#include "TriTools.h"

#if 0

XWinding *WindingForTri(const mapTri_t *tri)
{
	XWinding	*w;

	w = new XWinding(3);
	w->SetNumPoints(3);
	(*w)[0] = tri->v[0].pos;
	(*w)[1] = tri->v[1].pos;
	(*w)[2] = tri->v[2].pos;

	return w;
}

mapTri_t* CopyMapTri(const mapTri_t *tri)
{
	mapTri_t		*t;

	t = (mapTri_t *)malloc(sizeof(*t));
	*t = *tri;

	return t;
}


mapTri_t* MergeTriLists(mapTri_t *a, mapTri_t *b)
{
	mapTri_t	**prev;

	prev = &a;
	while (*prev) {
		prev = &(*prev)->next;
	}

	*prev = b;
	return a;
}

void FreeTriList(mapTri_t *a)
{
	mapTri_t	*next;

	for (; a; a = next) {
		next = a->next;
		free(a);
	}
}

int	CountTriList(const mapTri_t *tri)
{
	int		c;

	c = 0;
	while (tri) {
		c++;
		tri = tri->next;
	}

	return c;
}



mapTri_t* AllocTri(void)
{
	mapTri_t	*tri;

	tri = (mapTri_t *)malloc(sizeof(*tri));
	memset(tri, 0, sizeof(*tri));
	return tri;
}

/*
===============
FreeTri
===============
*/
void FreeTri(mapTri_t *tri) 
{
	free(tri);
}

/*
================
TriVertsFromOriginal

Regenerate the texcoords and colors on a fragmented tri from the plane equations
================
*/
void TriVertsFromOriginal(mapTri_t *tri, const mapTri_t *original)
{
	int		i, j;
	float	denom;

	denom = XWinding::TriangleArea(original->v[0].pos, original->v[1].pos, original->v[2].pos);
	if (denom == 0) {
		return;		// original was degenerate, so it doesn't matter
	}

	for (i = 0; i < 3; i++) {
		float	a, b, c;

		// find the barycentric coordinates
		a = XWinding::TriangleArea(tri->v[i].pos, original->v[1].pos, original->v[2].pos) / denom;
		b = XWinding::TriangleArea(tri->v[i].pos, original->v[2].pos, original->v[0].pos) / denom;
		c = XWinding::TriangleArea(tri->v[i].pos, original->v[0].pos, original->v[1].pos) / denom;

		// regenerate the interpolated values
		//	tri->v[i].st[0] = a * original->v[0].st[0]
		//		+ b * original->v[1].st[0] + c * original->v[2].st[0];
		//	tri->v[i].st[1] = a * original->v[0].st[1]
		//		+ b * original->v[1].st[1] + c * original->v[2].st[1];

		for (j = 0; j < 3; j++) {
			tri->v[i].normal[j] = a * original->v[0].normal[j]
				+ b * original->v[1].normal[j] + c * original->v[2].normal[j];
		}
		tri->v[i].normal.normalize();
	}
}

mapTri_t *WindingToTriList(const XWinding *w, const mapTri_t *originalTri)
{
	mapTri_t	*tri;
	mapTri_t	*triList;
	int			i, j;
	const Vec3f	*vec;

	if (!w) {
		return NULL;
	}

	triList = NULL;
	for (i = 2; i < w->GetNumPoints(); i++) {
		tri = AllocTri();
		if (!originalTri) {
			memset(tri, 0, sizeof(*tri));
		}
		else {
			*tri = *originalTri;
		}
		tri->next = triList;
		triList = tri;

		for (j = 0; j < 3; j++) {
			if (j == 0) {
				vec = &((*w)[0]);
			}
			else if (j == 1) {
				vec = &((*w)[i - 1]);
			}
			else {
				vec = &((*w)[i]);
			}

			tri->v[j].pos = *vec;
		}
		if (originalTri) {
			TriVertsFromOriginal(tri, originalTri);
		}
	}

	return triList;
}


#endif