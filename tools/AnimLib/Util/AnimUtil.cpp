#include "stdafx.h"
#include "AnimUtil.h"


X_NAMESPACE_BEGIN(anim)

namespace Util
{



	void transformBones(core::Array<Matrix44f, core::ArrayAlignedAllocatorFixed<Matrix44f, 16>>& mats, 
		const uint8_t* pParents, const int32_t firstJoint, const int32_t lastJoint)
	{
		X_ASSERT(lastJoint < safe_static_cast<int32_t>(mats.size()), "out of range")(lastJoint, mats.size());
		X_ASSERT(firstJoint <= lastJoint, "out of range")(firstJoint, lastJoint);

		for (int32_t i = firstJoint; i <= lastJoint; i++) {
			X_ASSERT(pParents[i] < mats.size(), "Parent out of range")(pParents[i]);
			mats[i] *= mats[pParents[i]];
		}
	}

	void blendBones(core::Array<Transformf>& bones, const core::Array<Transformf>& blendTrans,
		const core::Array<int32_t>& indexes, float lerp)
	{
		X_ASSERT(bones.size() == blendTrans.size(), "size mismatch")();

		int32_t num = safe_static_cast<int32_t>(indexes.size());
		for (int32_t i = 0; i < num; i++)
		{
			int32_t j = indexes[i];
			auto& dst = bones[j];
			auto& src = blendTrans[j];

			dst.quat = dst.quat.slerp(lerp, src.quat);
			dst.pos = dst.pos.lerp(lerp, src.pos);
		}
	}

	void convertBoneTransToMatrix(core::Array<Matrix44f, core::ArrayAlignedAllocatorFixed<Matrix44f, 16>>& mats,
		const core::Array<Transformf>& trans)
	{
		X_ASSERT(mats.size() == trans.size(), "size mismatch")();

		int32_t num = safe_static_cast<int32_t>(mats.size());
		for (int32_t i = 0; i < num; i++)
		{
			mats[i] = trans[i].quat.toMatrix44();
			mats[i].setTranslate(trans[i].pos);
		}
	}


} // namespace Util


X_NAMESPACE_END