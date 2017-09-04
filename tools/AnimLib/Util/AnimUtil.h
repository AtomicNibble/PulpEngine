#pragma once


X_NAMESPACE_BEGIN(anim)

namespace Util
{


	void blendBones(Transformf* pBones, const Transformf* pBlendBones, float lerp, const int32_t* pIndex, size_t numIndex);

	void convertBoneTransToMatrix(Matrix44f* pMats, const Transformf* pTrans, size_t numBones);

	void transformBones(Matrix44f* pMats, const int32_t* pParents, const int32_t firstJoint, const int32_t lastJoint);

} // namespace Util

X_NAMESPACE_END