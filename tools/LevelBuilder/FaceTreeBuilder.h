#pragma once

#ifndef X_FACE_TREE_BUILDER_H_
#define X_FACE_TREE_BUILDER_H_

#include "PlaneSet.h"

struct bspNode;
struct bspFace;

class FaceTreeBuilder
{
public:
	FaceTreeBuilder(XPlaneSet& planeset);

	bool Build(bspNode* pRootNode, bspFace* pFaceList);

	size_t getNumLeafs(void) const;

private:
	void BuildFaceTree_r(bspNode* node, bspFace* faces);

	int32_t SelectSplitPlaneNum(bspNode* node, bspFace* faces);
	int32_t FindFloatPlane(const Planef& plane);

private:
	XPlaneSet& planeset_;
	size_t numLeafs_;
};


#endif // !X_FACE_TREE_BUILDER_H_