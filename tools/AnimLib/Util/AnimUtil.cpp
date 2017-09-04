#include "stdafx.h"
#include "AnimUtil.h"


X_NAMESPACE_BEGIN(anim)

namespace Util
{


	void blendBones(Transformf* pBones, const Transformf* pBlendBones, float lerp, const int32_t* pIndex, size_t numIndex)
	{
		int32_t num = safe_static_cast<int32_t>(numIndex);
		for (int32_t i = 0; i < num; i++)
		{
			int32_t j = pIndex[i];
			pBones[j].quat.slerp(lerp, pBlendBones[j].quat);
			pBones[j].pos.lerp(lerp, pBlendBones[j].pos);
		}
	}

	void convertBoneTransToMatrix(Matrix44f* pMat, const Transformf* pTrans, size_t numBones)
	{
		int32_t num = safe_static_cast<int32_t>(numBones);
		for (int32_t i = 0; i < num; i++)
		{
			pMat[i] = pTrans[i].quat.toMatrix44();
			pMat[i].setTranslate(pTrans[i].pos);
		}
	}

	void transformBones(Matrix44f* pMats, const int32_t* pParents, const int32_t firstJoint, const int32_t lastJoint)
	{
		for (int32_t i = firstJoint; i <= lastJoint; i++) {
			X_ASSERT(pParents[i] < i, "Parent out of range")(pParents[i]);
			pMats[i] *= pMats[pParents[i]];
		}
	}


} // namespace Util


X_NAMESPACE_END