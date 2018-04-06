#pragma once

#include "Util\PlaneSet.h"

X_NAMESPACE_BEGIN(level)

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

X_NAMESPACE_END
