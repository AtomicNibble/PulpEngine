#pragma once


#include <Containers\Array.h>

X_NAMESPACE_BEGIN(anim)

namespace Util
{

	void transformBones(core::Array<Matrix44f, core::ArrayAlignedAllocatorFixed<Matrix44f, 16>>& mats,
		const uint8_t* pParents, const int32_t firstJoint, const int32_t lastJoint);

	void blendBones(core::Array<Transformf>& bones, const core::Array<Transformf>& blendTrans, 
		const core::Array<int32_t>& indexes, float lerp);

	void convertBoneTransToMatrix(core::Array<Matrix44f, core::ArrayAlignedAllocatorFixed<Matrix44f, 16>>& mats, 
		const core::Array<Transformf>& trans);

} // namespace Util

X_NAMESPACE_END