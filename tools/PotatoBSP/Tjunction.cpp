#include "stdafx.h"

#include <ITimer.h>

#include "BSPTypes.h"
#include "MapTypes.h"

#include "TriTools.h"

/*

T junction fixing never creates more xyz points, but
new vertexes will be created when different surfaces
cause a fix

The vertex cleaning accomplishes two goals: removing extranious low order
bits to avoid numbers like 1.000001233, and grouping nearby vertexes
together.  Straight truncation accomplishes the first foal, but two vertexes
only a tiny epsilon apart could still be spread to different snap points.
To avoid this, we allow the merge test to group points together that
snapped to neighboring integer coordinates.

Snaping verts can drag some triangles backwards or collapse them to points,
which will cause them to be removed.


When snapping to ints, a point can move a maximum of sqrt(3)/2 distance
Two points that were an epsilon apart can then become sqrt(3) apart

A case that causes recursive overflow with point to triangle fixing:

A
C            D
B

Triangle ABC tests against point D and splits into triangles ADC and DBC
Triangle DBC then tests against point A again and splits into ABC and ADB
infinite recursive loop


For a given source triangle
init the no-check list to hold the three triangle hashVerts

recursiveFixTriAgainstHash

recursiveFixTriAgainstHashVert_r
if hashVert is on the no-check list
exit
if the hashVert should split the triangle
add to the no-check list
recursiveFixTriAgainstHash(a)
recursiveFixTriAgainstHash(b)

*/

#if 0

#define	SNAP_FRACTIONS	32
#define	VERTEX_EPSILON	( 1.0 / SNAP_FRACTIONS )

#define	COLINEAR_EPSILON	( 1.8 * VERTEX_EPSILON )

#define	HASH_BINS	16

typedef struct hashVert_s {
	struct hashVert_s	*next;
	Vec3f				v;
	int					iv[3];
} hashVert_t;

static AABB		hashBounds;
static Vec3f	hashScale;
static hashVert_t	*hashVerts[HASH_BINS][HASH_BINS][HASH_BINS];
static int		numHashVerts, numTotalVerts;
static int		hashIntMins[3], hashIntScale[3];



/*
===============
GetHashVert

Also modifies the original vert to the snapped value
===============
*/
struct hashVert_s* GetHashVert(Vec3f& v) 
{
	int		iv[3];
	int		block[3];
	int		i;
	hashVert_t	*hv;

	numTotalVerts++;

	// snap the vert to integral values
	for (i = 0; i < 3; i++) {
		iv[i] = (int)math<float>::floor((v[i] + 0.5f / SNAP_FRACTIONS) * SNAP_FRACTIONS);
		block[i] = (iv[i] - hashIntMins[i]) / hashIntScale[i];
		if (block[i] < 0) {
			block[i] = 0;
		}
		else if (block[i] >= HASH_BINS) {
			block[i] = HASH_BINS - 1;
		}
	}

	// see if a vertex near enough already exists
	// this could still fail to find a near neighbor right at the hash block boundary
	for (hv = hashVerts[block[0]][block[1]][block[2]]; hv; hv = hv->next) {
#if 0
		if (hv->iv[0] == iv[0] && hv->iv[1] == iv[1] && hv->iv[2] == iv[2]) {
			VectorCopy(hv->v, v);
			return hv;
		}
#else
		for (i = 0; i < 3; i++) {
			int	d;
			d = hv->iv[i] - iv[i];
			if (d < -1 || d > 1) {
				break;
			}
		}
		if (i == 3) {
			v = hv->v;
			return hv;
		}
#endif
	}

	// create a new one 
	hv = (hashVert_t *)malloc(sizeof(*hv));

	hv->next = hashVerts[block[0]][block[1]][block[2]];
	hashVerts[block[0]][block[1]][block[2]] = hv;

	hv->iv[0] = iv[0];
	hv->iv[1] = iv[1];
	hv->iv[2] = iv[2];

	hv->v[0] = (float)iv[0] / SNAP_FRACTIONS;
	hv->v[1] = (float)iv[1] / SNAP_FRACTIONS;
	hv->v[2] = (float)iv[2] / SNAP_FRACTIONS;

	v = hv->v;

	numHashVerts++;

	return hv;
}


/*
==================
HashBlocksForTri

Returns an inclusive bounding box of hash
bins that should hold the triangle
==================
*/
static void HashBlocksForTri(const mapTri_t *tri, int blocks[2][3])
{
	AABB	bounds;
	int			i;

	bounds.clear();
	bounds.add(tri->v[0].pos);
	bounds.add(tri->v[1].pos);
	bounds.add(tri->v[2].pos);

	// add a 1.0 slop margin on each side
	for (i = 0; i < 3; i++) {
		blocks[0][i] = (int)((bounds.min[i] - 1.0f - ((float)hashBounds.min[i])) / hashScale[i]);
		if (blocks[0][i] < 0) {
			blocks[0][i] = 0;
		}
		else if (blocks[0][i] >= HASH_BINS) {
			blocks[0][i] = HASH_BINS - 1;
		}

		blocks[1][i] = (int)((bounds.max[i] + 1.0f - ((float)hashBounds.min[i])) / hashScale[i]);
		if (blocks[1][i] < 0) {
			blocks[1][i] = 0;
		}
		else if (blocks[1][i] >= HASH_BINS) {
			blocks[1][i] = HASH_BINS - 1;
		}
	}
}


/*
=================
HashTriangles

Removes triangles that are degenerated or flipped backwards
=================
*/
void HashTriangles(optimizeGroup_t *groupList) 
{
	mapTri_t	*a;
	int			vert;
	int			i;
	optimizeGroup_t	*group;

	// clear the hash tables
	memset(hashVerts, 0, sizeof(hashVerts));

	numHashVerts = 0;
	numTotalVerts = 0;

	// bound all the triangles to determine the bucket size
	hashBounds.clear();
	for (group = groupList; group; group = group->nextGroup) {
		for (a = group->triList; a; a = a->next) {
			hashBounds.add(a->v[0].pos);
			hashBounds.add(a->v[1].pos);
			hashBounds.add(a->v[2].pos);
		}
	}

	// spread the bounds so it will never have a zero size
	for (i = 0; i < 3; i++) {
		hashBounds.min[i] = floor(hashBounds.min[i] - 1);
		hashBounds.max[i] = ceil(hashBounds.max[i] + 1);
		hashIntMins[i] = (int)(hashBounds.min[i] * SNAP_FRACTIONS);

		hashScale[i] = (hashBounds.max[i] - hashBounds.min[i]) / HASH_BINS;
		hashIntScale[i] = (int)(hashScale[i] * SNAP_FRACTIONS);
		if (hashIntScale[i] < 1) {
			hashIntScale[i] = 1;
		}
	}

	// add all the points to the hash buckets
	for (group = groupList; group; group = group->nextGroup) {
		// don't create tjunctions against discrete surfaces (blood decals, etc)
#if 0
		if (group->material != NULL && group->material->IsDiscrete()) {
			continue;
		}
#endif
		for (a = group->triList; a; a = a->next) {
			for (vert = 0; vert < 3; vert++) {
				a->hashVert[vert] = GetHashVert(a->v[vert].pos);
			}
		}
	}
}

/*
=================
FreeTJunctionHash

The optimizer may add some more crossing verts
after t junction processing
=================
*/
void FreeTJunctionHash(void) 
{
	int			i, j, k;
	hashVert_t	*hv, *next;

	for (i = 0; i < HASH_BINS; i++) {
		for (j = 0; j < HASH_BINS; j++) {
			for (k = 0; k < HASH_BINS; k++) {
				for (hv = hashVerts[i][j][k]; hv; hv = next) {
					next = hv->next;
					free(hv);
				}
			}
		}
	}
	memset(hashVerts, 0, sizeof(hashVerts));
}


/*
==================
FixTriangleAgainstHashVert

Returns a list of two new mapTri if the hashVert is
on an edge of the given mapTri, otherwise returns NULL.
==================
*/
mapTri_t *FixTriangleAgainstHashVert(const mapTri_t *a, const hashVert_t *hv) 
{
	int			i;
	const xVert	*v1, *v2, *v3;
	xVert		split;
	Vec3f		dir;
	float		len;
	float		frac;
	mapTri_t	*new1, *new2;
	Vec3f		temp;
	float		d, off;
	const Vec3f *v;
	Planef		plane1, plane2;

	v = &hv->v;

	// if the triangle already has this hashVert as a vert,
	// it can't be split by it
	if (a->hashVert[0] == hv || a->hashVert[1] == hv || a->hashVert[2] == hv) {
		return NULL;
	}

	// we probably should find the edge that the vertex is closest to.
	// it is possible to be < 1 unit away from multiple
	// edges, but we only want to split by one of them
	for (i = 0; i < 3; i++) 
	{
		v1 = &a->v[i];
		v2 = &a->v[(i + 1) % 3];
		v3 = &a->v[(i + 2) % 3];
	//	VectorSubtract(v2->xyz, v1->xyz, dir);
		dir = v2->pos - v1->pos;
		len = dir.length();
		dir.normalize();

		// if it is close to one of the edge vertexes, skip it
	//	VectorSubtract(*v, v1->xyz, temp);
		temp = *v - v1->pos;
		d = temp.dot(dir);
		if (d <= 0 || d >= len) {
			continue;
		}

		// make sure it is on the line
		temp = v1->pos + dir * d;
		temp -= *v;

	//	VectorMA(v1->xyz, d, dir, temp);
	//	VectorSubtract(temp, *v, temp);
		off = temp.length();
		if (off <= -COLINEAR_EPSILON || off >= COLINEAR_EPSILON) {
			continue;
		}

		// take the x/y/z from the splitter,
		// but interpolate everything else from the original tri
		split.pos = *v;
		frac = d / len;
		split.uv[0] = v1->uv[0] + frac * (v2->uv[0] - v1->uv[0]);
		split.uv[1] = v1->uv[1] + frac * (v2->uv[1] - v1->uv[1]);
		split.normal[0] = v1->normal[0] + frac * (v2->normal[0] - v1->normal[0]);
		split.normal[1] = v1->normal[1] + frac * (v2->normal[1] - v1->normal[1]);
		split.normal[2] = v1->normal[2] + frac * (v2->normal[2] - v1->normal[2]);
		split.normal.normalize();

		// split the tri
		new1 = CopyMapTri(a);
		new1->v[(i + 1) % 3] = split;
		new1->hashVert[(i + 1) % 3] = hv;
		new1->next = NULL;

		new2 = CopyMapTri(a);
		new2->v[i] = split;
		new2->hashVert[i] = hv;
		new2->next = new1;

		plane1 = Planef(new1->hashVert[0]->v, new1->hashVert[1]->v, new1->hashVert[2]->v);
		plane2 = Planef(new2->hashVert[0]->v, new2->hashVert[1]->v, new2->hashVert[2]->v);

		d = plane1.dot(plane2);

		// if the two split triangle's normals don't face the same way,
		// it should not be split
		if (d <= 0) {
			FreeTriList(new2);
			continue;
		}

		return new2;
	}


	return NULL;
}



/*
==================
FixTriangleAgainstHash

Potentially splits a triangle into a list of triangles based on tjunctions
==================
*/
mapTri_t* FixTriangleAgainstHash(const mapTri_t *tri) 
{
	mapTri_t		*fixed;
	mapTri_t		*a;
	mapTri_t		*test, *next;
	int				blocks[2][3];
	int				i, j, k;
	hashVert_t		*hv;

	// if this triangle is degenerate after point snapping,
	// do nothing (this shouldn't happen, because they should
	// be removed as they are hashed)
	if (tri->hashVert[0] == tri->hashVert[1]
		|| tri->hashVert[0] == tri->hashVert[2]
		|| tri->hashVert[1] == tri->hashVert[2]) {
		return NULL;
	}

	fixed = CopyMapTri(tri);
	fixed->next = NULL;

	HashBlocksForTri(tri, blocks);
	for (i = blocks[0][0]; i <= blocks[1][0]; i++) {
		for (j = blocks[0][1]; j <= blocks[1][1]; j++) {
			for (k = blocks[0][2]; k <= blocks[1][2]; k++) {
				for (hv = hashVerts[i][j][k]; hv; hv = hv->next) {
					// fix all triangles in the list against this point
					test = fixed;
					fixed = NULL;
					for (; test; test = next) {
						next = test->next;
						a = FixTriangleAgainstHashVert(test, hv);
						if (a) {
							// cut into two triangles
							a->next->next = fixed;
							fixed = a;
							FreeTri(test);
						}
						else {
							test->next = fixed;
							fixed = test;
						}
					}
				}
			}
		}
	}

	return fixed;
}


/*
==================
CountGroupListTris
==================
*/
int CountGroupListTris(const optimizeGroup_t *groupList) 
{
	int		c;

	c = 0;
	for (; groupList; groupList = groupList->nextGroup) {
		c += CountTriList(groupList->triList);
	}

	return c;
}

/*
==================
FixAreaGroupsTjunctions
==================
*/
void FixAreaGroupsTjunctions(optimizeGroup_t *groupList) 
{
	const mapTri_t	*tri;
	mapTri_t		*newList;
	mapTri_t		*fixed;
	int				startCount, endCount;
	optimizeGroup_t	*group;

	if (gSettings.noTJunc) {
		return;
	}

	if (!groupList) {
		return;
	}

	startCount = CountGroupListTris(groupList);


	X_LOG0("Tjunc", "----- FixAreaGroupsTjunctions -----");
	X_LOG0("Tjunc", "%6i triangles in", startCount);
	

	HashTriangles(groupList);

	for (group = groupList; group; group = group->nextGroup) {
		// don't touch discrete surfaces
#if 0
		if (group->material != NULL && group->material->IsDiscrete()) {
			continue;
		}
#endif

		newList = NULL;
		for (tri = group->triList; tri; tri = tri->next) {
			fixed = FixTriangleAgainstHash(tri);
			newList = MergeTriLists(newList, fixed);
		}
		FreeTriList(group->triList);
		group->triList = newList;
	}

	endCount = CountGroupListTris(groupList);

	X_LOG0("Tjunc", "%6i triangles out", endCount);
}



void BSPData::FixEntityTjunctions(uEntity_t *e)
{
	int		i;

	for (i = 0; i < e->numAreas; i++) {
		FixAreaGroupsTjunctions(e->areas[i].groups);
		FreeTJunctionHash();
	}
}


void BSPData::FixGlobalTjunctions(uEntity_t *e)
{
	mapTri_t	*a;
	int			vert;
	int			i;
	optimizeGroup_t	*group;
	int			areaNum;

	X_LOG0("TJunc", "----- FixGlobalTjunctions -----");

	// clear the hash tables
	memset(hashVerts, 0, sizeof(hashVerts));

	numHashVerts = 0;
	numTotalVerts = 0;

	// bound all the triangles to determine the bucket size
	hashBounds.clear();
	for (areaNum = 0; areaNum < e->numAreas; areaNum++) {
		for (group = e->areas[areaNum].groups; group; group = group->nextGroup) {
			for (a = group->triList; a; a = a->next) {
				hashBounds.add(a->v[0].pos);
				hashBounds.add(a->v[1].pos);
				hashBounds.add(a->v[2].pos);
			}
		}
	}

	// spread the bounds so it will never have a zero size
	for (i = 0; i < 3; i++) {
		hashBounds.min[i] = math<float>::floor(hashBounds.min[i] - 1);
		hashBounds.max[i] = math<float>::ceil(hashBounds.max[i] + 1);
		hashIntMins[i] = (int)(hashBounds.min[i] * SNAP_FRACTIONS);

		hashScale[i] = (hashBounds.max[i] - hashBounds.min[i]) / HASH_BINS;
		hashIntScale[i] = (int)(hashScale[i] * SNAP_FRACTIONS);
		if (hashIntScale[i] < 1) {
			hashIntScale[i] = 1;
		}
	}

	// add all the points to the hash buckets
	for (areaNum = 0; areaNum < e->numAreas; areaNum++) {
		for (group = e->areas[areaNum].groups; group; group = group->nextGroup) {
			// don't touch discrete surfaces
#if 0
			if (group->material != NULL && group->material->IsDiscrete()) {
				continue;
			}
#endif

			for (a = group->triList; a; a = a->next) {
				for (vert = 0; vert < 3; vert++) {
					a->hashVert[vert] = GetHashVert(a->v[vert].pos);
				}
			}
		}
	}

	// add all the func_static model vertexes to the hash buckets
	// optionally inline some of the func_static models
#if 0
	if (numEntities == 0)
	{
		for (int eNum = 1; eNum < numEntities; eNum++) {
			uEntity_t *entity = &entities[eNum];
			const char *className = entity->mapEntity->epairs.GetString("classname");
			if (idStr::Icmp(className, "func_static")) {
				continue;
			}
			const char *modelName = entity->mapEntity->epairs.GetString("model");
			if (!modelName) {
				continue;
			}
			if (!strstr(modelName, ".lwo") && !strstr(modelName, ".ase") && !strstr(modelName, ".ma")) {
				continue;
			}

			idRenderModel	*model = renderModelManager->FindModel(modelName);

			//			common->Printf( "adding T junction verts for %s.\n", entity->mapEntity->epairs.GetString( "name" ) );

			idMat3	axis;
			// get the rotation matrix in either full form, or single angle form
			if (!entity->mapEntity->epairs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1", axis)) {
				float angle = entity->mapEntity->epairs.GetFloat("angle");
				if (angle != 0.0f) {
					axis = idAngles(0.0f, angle, 0.0f).ToMat3();
				}
				else {
					axis.Identity();
				}
			}

			Vec3f	origin = entity->mapEntity->epairs.GetVector("origin");

			for (i = 0; i < model->NumSurfaces(); i++) {
				const modelSurface_t *surface = model->Surface(i);
				const srfTriangles_t *tri = surface->geometry;

				mapTri_t	mapTri;
				memset(&mapTri, 0, sizeof(mapTri));
				mapTri.material = surface->shader;
				// don't let discretes (autosprites, etc) merge together
				if (mapTri.material->IsDiscrete()) {
					mapTri.mergeGroup = (void *)surface;
				}
				for (int j = 0; j < tri->numVerts; j += 3) {
					idVec3 v = tri->verts[j].xyz * axis + origin;
					GetHashVert(v);
				}
			}
		}
	}
#endif


	// now fix each area
	for (areaNum = 0; areaNum < e->numAreas; areaNum++) 
	{
		for (group = e->areas[areaNum].groups; group; group = group->nextGroup) {
			// don't touch discrete surfaces
#if 0
			if (group->material != NULL && group->material->IsDiscrete()) {
				continue;
			}
#endif
			mapTri_t *newList = NULL;
			for (mapTri_t *tri = group->triList; tri; tri = tri->next) {
				mapTri_t *fixed = FixTriangleAgainstHash(tri);
				newList = MergeTriLists(newList, fixed);
			}
			FreeTriList(group->triList);
			group->triList = newList;
		}
	}


	// done
	FreeTJunctionHash();
}


#endif