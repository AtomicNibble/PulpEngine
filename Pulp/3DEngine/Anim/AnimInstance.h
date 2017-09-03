#pragma once

#include <IAnimation.h>
#include <Time\TimeVal.h>

#if 0

X_NAMESPACE_BEGIN(anim)

class Anim;


class AnimInstance
{
	template<typename T>
	using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

	typedef AlignedArray<Matrix44f> Mat44Arr;

public:
	AnimInstance(const Anim& anim, core::MemoryArenaBase* arena);

	void update(core::TimeVal delta);

	const Mat44Arr& getBoneMats(void) const;

private:
	const Anim& anim_;
	core::MemoryArenaBase* arena_;

	AnimState state_;
	Mat44Arr boneMat_;
};

X_NAMESPACE_END

#endif