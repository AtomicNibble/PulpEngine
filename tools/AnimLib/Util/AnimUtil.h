#pragma once


#include <Containers\Array.h>

X_NAMESPACE_BEGIN(anim)

namespace Util
{


	void blendBones(Transformf* pBones, const Transformf* pBlendBones, float lerp, const int32_t* pIndex, size_t numIndex);
	void transformBones(Matrix44f* pMats, const int32_t* pParents, const int32_t firstJoint, const int32_t lastJoint);

	void convertBoneTransToMatrix(core::Array<Matrix44f, core::ArrayAlignedAllocatorFixed<Matrix44f, 16>>& mats, 
		const core::Array<Transformf>& trans);

} // namespace Util

X_NAMESPACE_END