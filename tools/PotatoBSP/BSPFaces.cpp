#include "stdafx.h"

#include <ITimer.h>
#include <IProfile.h>


#include "BSPTypes.h"

namespace
{
	static const float CLIP_EPSILON = 0.1f;

}

bspFace* BSPBuilder::MakeStructuralBspFaceList(bspBrush* list)
{
	int i;
	bspBrush*	b;
	BspSide*	s;
	XWinding*   w;
	bspFace     *f, *flist;


	flist = nullptr;
	for (b = list; b != nullptr; b = b->next)
	{
		if ( b->detail ) {
			continue;
		}

		for ( i = 0; i < b->numsides; i++ )
		{
			/* get side and winding */
			s = &b->sides[ i ];
			w = s->pWinding;

			// no winding O.O
			if ( w == nullptr ) 
				continue;	


			/* allocate a face */
			f = AllocBspFace();
			f->w = w->Copy();
			f->planenum = s->planenum & ~1;


			// add to list.
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}

bspTree* BSPBuilder::FaceBSP(bspFace* list)
{
	X_ASSERT_NOT_NULL(list);

	bspTree	*	tree;
	bspFace*    face;
	int			i;
	int			count;

	core::TimeVal start = gEnv->pTimer->GetAsyncTime();

	X_LOG_BULLET;
	// X_LOG0("BSP", "--- FaceBSP ---");

	tree = AllocTree();

	count = 0;
	for (face = list; face; face = face->next) {
		count++;
		for (i = 0; i < face->w->GetNumPoints(); i++) {
			tree->bounds.add((*face->w)[i]);
		}
	}
	X_LOG0("BSP", "%5i faces", count);

	tree->headnode = AllocNode();
	tree->headnode->bounds = tree->bounds;

	BuildFaceTree_r(tree->headnode, list);

	X_LOG0("BSP", "%5i leafs", stats_.numFaceLeafs);

	core::TimeVal end = gEnv->pTimer->GetAsyncTime();

	X_LOG0("BSP", "%5.1f ms faceBsp", (end - start).GetMilliSeconds());

	return tree;

}

void BSPBuilder::BuildFaceTree_r(bspNode* node, bspFace* list)
{
	bspFace     *split;
	bspFace		*next;
	Planeside::Enum side;
	Planef		*plane;
	bspFace     *newFace;
	bspFace     *childLists[2];
	XWinding	*frontWinding, *backWinding;
	int i;
	int splitPlaneNum;


	// count faces left 
	i = CountFaceList( list );

	// select the best split plane 
	SelectSplitPlaneNum( node, list, &splitPlaneNum );

	// if we don't have any more faces, this is a node 
	if ( splitPlaneNum == -1 ) {
		node->planenum = PLANENUM_LEAF;
		stats_.numFaceLeafs++;
		return;
	}

	// partition the list 
	node->planenum = splitPlaneNum;

	plane = &planes[ splitPlaneNum ];
	childLists[0] = NULL;
	childLists[1] = NULL;

	for ( split = list; split; split = next )
	{
		// set next 
		next = split->next;

		// don't split by identical plane 
		if ( split->planenum == node->planenum ) {
			FreeBspFace( split );
			continue;
		}

		// determine which side the face falls on 
		side = split->w->PlaneSide( *plane );

		// switch on side 
		if (side == Planeside::CROSS)
		{
			split->w->Split(*plane, CLIP_EPSILON * 2, &frontWinding, &backWinding);

			if ( frontWinding ) {
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				childLists[0] = newFace;
			}
			if ( backWinding ) {
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				childLists[1] = newFace;
			}
			FreeBspFace( split );
		}
		else if (side == Planeside::FRONT) {
			split->next = childLists[0];
			childLists[0] = split;
		}
		else if (side == Planeside::BACK) {
			split->next = childLists[1];
			childLists[1] = split;
		}
	}


	// recursively process children
	for ( i = 0 ; i < 2 ; i++ ) {
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		if ( plane->getNormal()[i] == 1 ) 
		{
			node->children[0]->bounds.min[i] = plane->getDistance();
			node->children[1]->bounds.max[i] = plane->getDistance();
			break;
		}
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		BuildFaceTree_r( node->children[i], childLists[i] );
	}
}



int BSPBuilder::CountFaceList(bspFace* list)
{
	int c;
	c = 0;
	for ( ; list != nullptr; list = list->next )
		c++;
	return c;
}


void BSPBuilder::SelectSplitPlaneNum(bspNode* node, bspFace* list, int *splitPlaneNum)
{
	bspFace      *split;
	bspFace      *check;
	bspFace      *bestSplit;
	int splits, facing, front, back;
	Planeside::Enum side;
	Planef     *plane;
	int value, bestValue;
	int i;
	Vec3f normal;
	float dist;
	int planenum;


	*splitPlaneNum = -1; // leaf 

	for (i = 0; i < 3; i++)
	{
		if (blockSize_[i] <= 0) {
			continue;
		}

		dist = blockSize_[i] * (floor(node->bounds.min[i] / blockSize_[i]) + 1);
		if (node->bounds.max[i] > dist) 
		{
			normal = Vec3f::zero();
			normal[i] = 1;
			planenum = FindFloatPlane(Planef(normal, dist));
			*splitPlaneNum = planenum;
			return;
		}
	}

	// pick one of the face planes 
	bestValue = -99999;
	bestSplit = list;

	for (split = list; split; split = split->next)
		split->checked = false;

	for (split = list; split; split = split->next)
	{
		if (split->checked) {
			continue;
		}

		plane = &planes[split->planenum];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for (check = list; check; check = check->next) 
		{
			if (check->planenum == split->planenum) {
				facing++;
				check->checked = true; // won't need to test this plane again
				continue;
			}

			side = check->w->PlaneSide(*plane);
			if (side == Planeside::CROSS) {
				splits++;
			}
			else if (side == Planeside::FRONT) {
				front++;
			}
			else if (side == Planeside::BACK) {
				back++;
			}
		}

		value = 5 * facing - 5 * splits; // - abs(front-back);
		if (PlaneType::isTrueAxial(plane->getType())) {
			value += 5;       // axial is better
		}
//		value += split->priority;       // prioritize hints higher

		if (value > bestValue) {
			bestValue = value;
			bestSplit = split;
		}
	}

	// nothing, we have a leaf 
	if (bestValue == -99999) {
		return;
	}

	// set best split data 
	*splitPlaneNum = bestSplit->planenum;
}







#if 0

int	 c_faceLeafs;


bspface_t* AllocBspFace(void) 
{
	bspface_t	*f;

	f = (bspface_t*)malloc(sizeof(*f));
	core::zero_this(f);
	return f;
}

void FreeBspFace(bspface_t* f) 
{
	if (f->w) {
		delete f->w;
	}
	free(f);
}

tree_t *AllocTree(void)
{
	tree_t	*tree;

	tree = (tree_t *)malloc(sizeof(*tree));
	core::zero_this(tree);
	tree->bounds.clear();
	return tree;
}

node_t *AllocNode(void)
{
	node_t	*node;

	node = (node_t *)malloc(sizeof(*node));
	core::zero_this(node);
	node->bounds.clear();
	return node;
}



bspface_t* BSPData::MakeStructuralBspFaceList(primitive_t *list)
{
	uBrush_t	*b;
	int			i;
	side_t		*s;
	XWinding	*w;
	bspface_t	*f, *flist;

	flist = NULL;
	for (; list; list = list->next) {
		b = list->brush;
		if (!b) {
			continue;
		}
		if (!b->opaque) {
			continue;
		}
		for (i = 0; i < b->numsides; i++) {
			s = &b->sides[i];
			w = s->winding;
			if (!w) {
				continue;
			}

			f = AllocBspFace();
			/*if (s->material->GetContentFlags() & CONTENTS_AREAPORTAL) {
				f->portal = true;
			}*/

			f->w = w->Copy();
			f->planenum = s->planenum & ~1;
			f->next = flist;
			flist = f;
		}
	}

	return flist;
}

#define	BLOCK_SIZE	1024.f

int BSPData::SelectSplitPlaneNum(node_t *node, bspface_t *list)
{
	bspface_t	*split;
	bspface_t	*check;
	bspface_t	*bestSplit;
	int			splits, facing, front, back;
	int			side;
	Planef		*mapPlane;
	int			value, bestValue;
	Planef		plane;
	int			planenum;
	bool	havePortals;
	float		dist;
	Vec3f		halfSize;

	// if it is crossing a 1k block boundary, force a split
	// this prevents epsilon problems from extending an
	// arbitrary distance across the map

	halfSize = node->bounds.size() * 0.5f;
	for (int axis = 0; axis < 3; axis++) 
	{
		if (halfSize[axis] > BLOCK_SIZE) {

			float test = (node->bounds.min[axis] + halfSize[axis]);
			test /= BLOCK_SIZE;
			test = math<float>::floor(test);
			test += 1.0f;
			test *= BLOCK_SIZE;


			dist = BLOCK_SIZE * (math<float>::floor((node->bounds.min[axis] + halfSize[axis]) / BLOCK_SIZE) + 1.0f);
		}
		else {
			dist = BLOCK_SIZE * (math<float>::floor(node->bounds.min[axis] / BLOCK_SIZE) + 1.0f);
		}
		if (dist > node->bounds.min[axis] + 1.0f && dist < node->bounds.max[axis] - 1.0f) 
		{
			plane.setNormal( Vec3f::zero() );
			plane[axis] = 1.0f;
			plane.setDistance(dist);
			planenum = FindFloatPlane(plane);
			return planenum;
		}
	}

	// pick one of the face planes
	// if we have any portal faces at all, only
	// select from them, otherwise select from
	// all faces
	bestValue = -999999;
	bestSplit = list;

	havePortals = false;
	for (split = list; split; split = split->next) {
		split->checked = false;
		if (split->portal) {
			havePortals = true;
		}
	}

	for (split = list; split; split = split->next) {
		if (split->checked) {
			continue;
		}
		if (havePortals != split->portal) {
			continue;
		}
		mapPlane = &planes[split->planenum];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for (check = list; check; check = check->next) {
			if (check->planenum == split->planenum) {
				facing++;
				check->checked = true;	// won't need to test this plane again
				continue;
			}
			side = check->w->PlaneSide(*mapPlane);
			if (side == SIDE_CROSS) {
				splits++;
			}
			else if (side == SIDE_FRONT) {
				front++;
			}
			else if (side == SIDE_BACK) {
				back++;
			}
		}
		value = 5 * facing - 5 * splits; // - abs(front-back);
		if (PlaneType::isTrueAxial(mapPlane->getType())) {
			value += 5;		// axial is better
		}

		if (value > bestValue) {
			bestValue = value;
			bestSplit = split;
		}
	}

	if (bestValue == -999999) {
		return -1;
	}

	return bestSplit->planenum;
}


void BSPData::BuildFaceTree_r(node_t *node, bspface_t *list)
{
	bspface_t	*split;
	bspface_t	*next;
	int			side;
	bspface_t	*newFace;
	bspface_t	*childLists[2];
	XWinding	*frontWinding, *backWinding;
	int			i;
	int			splitPlaneNum;

	static int call_num = 0;

	call_num++;

	splitPlaneNum = SelectSplitPlaneNum(node, list);
	// if we don't have any more faces, this is a node
	if (splitPlaneNum == -1) {
		node->planenum = PLANENUM_LEAF;
		c_faceLeafs++;
		return;
	}


	// partition the list
	node->planenum = splitPlaneNum;
	Planef &plane = this->planes[splitPlaneNum];
	childLists[0] = NULL;
	childLists[1] = NULL;

	{
	//	X_PROFILE_BEGIN("list partition");

		for (split = list; split; split = next) {
			next = split->next;

			if (split->planenum == node->planenum) {
				FreeBspFace(split);
				continue;
			}

			side = split->w->PlaneSide(plane);

			if (side == SIDE_CROSS) {
				split->w->Split(plane, CLIP_EPSILON * 2, &frontWinding, &backWinding);
				if (frontWinding) {
					newFace = AllocBspFace();
					newFace->w = frontWinding;
					newFace->next = childLists[0];
					newFace->planenum = split->planenum;
					childLists[0] = newFace;
				}
				if (backWinding) {
					newFace = AllocBspFace();
					newFace->w = backWinding;
					newFace->next = childLists[1];
					newFace->planenum = split->planenum;
					childLists[1] = newFace;
				}
				FreeBspFace(split);
			}
			else if (side == SIDE_FRONT) {
				split->next = childLists[0];
				childLists[0] = split;
			}
			else if (side == SIDE_BACK) {
				split->next = childLists[1];
				childLists[1] = split;
			}
		}

	}

	// recursively process children
	for (i = 0; i < 2; i++) {
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		node->children[i]->bounds = node->bounds;
	}

	// split the bounds if we have a nice axial plane
	for (i = 0; i < 3; i++) {
		if (math<float>::abs(plane.getNormal()[i] - 1.f) < 0.001f) {
			node->children[0]->bounds.min[i] = plane.getDistance();
			node->children[1]->bounds.max[i] = plane.getDistance();
			break;
		}
	}

	for (i = 0; i < 2; i++) {
		BuildFaceTree_r(node->children[i], childLists[i]);
	}
}



tree_t* BSPData::FaceBSP(bspface_t *list)
{
	tree_t		*tree;
	bspface_t	*face;
	int			i;
	int			count;

	core::TimeVal start = gEnv->pTimer->GetAsyncTime();
	
	X_LOG_BULLET;
	// X_LOG0("BSP", "--- FaceBSP ---");

	tree = AllocTree();

	count = 0;
	for (face = list; face; face = face->next) {
		count++;
		for (i = 0; i < face->w->GetNumPoints(); i++) {
			tree->bounds.add((*face->w)[i]);
		}
	}
	X_LOG0("BSP", "%5i faces", count);

	tree->headnode = AllocNode();
	tree->headnode->bounds = tree->bounds;

	c_faceLeafs = 0;

	BuildFaceTree_r(tree->headnode, list);

	X_LOG0("BSP", "%5i leafs", c_faceLeafs);

	core::TimeVal end = gEnv->pTimer->GetAsyncTime();

	X_LOG0("BSP", "%5.1f ms faceBsp", (end - start).GetMilliSeconds());

	return tree;
}

#endif