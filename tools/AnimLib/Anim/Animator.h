#pragma once

#include <Containers\Array.h>

#include <Time\TimeVal.h>

X_NAMESPACE_DECLARE(model, class XModel)

X_NAMESPACE_BEGIN(anim)


// this will animate a model
// you can add multiple anims to the animator and it will blend them for you.

// we need to support like a que of animations that are been blended.
// pushing a new anim on pushes the bottom on off.
// and we need to drop of animations that have finished blending out.
// but also a animation that has finished and not looping needs to be dropped.


class Anim;

// This handles blending a animation.
// It stores info for when the animation 
class AnimBlend
{
	typedef core::Array<Transformf> TransformArr;
	typedef core::Array<int32_t> IndexArr;

public:
	AnimBlend(core::MemoryArenaBase* arena);

	void clear(void);
	void clear(core::TimeVal currentTime, core::TimeVal clearTime);

	void playAnim(const model::XModel& model, Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime);
	
	bool blend(core::TimeVal currentTime, TransformArr& boneTransOut, float &blendWeight) const;
	
	bool isDone(core::TimeVal currentTime) const;
	float getWeight(core::TimeVal currentTime) const;

	core::TimeVal animTime(core::TimeVal currentTime) const;

	X_INLINE float getStartWeight(void) const;
	X_INLINE float getFinalWeight(void) const;
	X_INLINE core::TimeVal getStartTime(void) const;

private:
	Anim* pAnim_;

	core::TimeVal startTime_;
	core::TimeVal endTime_;

	core::TimeVal blendStart_;
	core::TimeVal blendDuration_;

	// start and end blend weights for anim
	float blendStartVal_;
	float blendEndVal_;

	float rate_; // 1.0 is default playback rate.

	IndexArr indexMap_;
};


class Animator
{
	template<typename T>
	using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

	template<typename T>
	using ArrayMulGrow = core::Array<T, core::ArrayAllocator<T>, core::growStrat::Multiply>;

	typedef AlignedArray<Matrix44f> Mat44Arr;
	typedef ArrayMulGrow<Anim*> AnimPtrArr;
	typedef core::Array<Transformf> TransformArr;

	static const size_t MAX_ANIMS_PER_CHANNEL = 2;

	typedef std::array<AnimBlend, MAX_ANIMS_PER_CHANNEL> AnimBlendArr;


public:
	ANIMLIB_EXPORT Animator(const model::XModel& model, core::MemoryArenaBase* arena);

	X_INLINE size_t numAnims(void) const;
	
	ANIMLIB_EXPORT bool createFrame(core::TimeVal currentTime);

	ANIMLIB_EXPORT void playAnim(Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime);
	ANIMLIB_EXPORT bool isAnimating(core::TimeVal currentTime) const;



private:
	void pushAnims(core::TimeVal currentTime, core::TimeVal blendTime);

private:
	core::MemoryArenaBase* arena_;
	const model::XModel& model_;

	core::TimeVal lastTransformTime_;

	AnimBlendArr anims_;
	Mat44Arr boneMat_;
};


X_NAMESPACE_END

#include "Animator.inl"