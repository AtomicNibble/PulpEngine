#pragma once

#include <IModel.h>

#include <Containers\Array.h>
#include <Time\TimeVal.h>

X_NAMESPACE_DECLARE(model, class XModel)

X_NAMESPACE_DECLARE(engine, class IPrimativeContext)

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

	void playAnim(const model::XModel& model, const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime);
	
	bool blend(core::TimeVal currentTime, TransformArr& boneTransOut, float &blendWeight) const;
	void blendOrigin(core::TimeVal currentTime, Vec3f& blendPos, float& blendWeight) const;
	void blendDelta(core::TimeVal fromTime, core::TimeVal toTime, Vec3f& blendDelta, float& blendWeight) const;

	bool isDone(core::TimeVal currentTime) const;
	float getWeight(core::TimeVal currentTime) const;

	core::TimeVal animTime(core::TimeVal currentTime) const;

	void setCycleCount(int32_t numCycles);
	void setRate(float rate);
	void setWeight(float newWeight, core::TimeVal currentTime, core::TimeVal blendTime);


	X_INLINE const Anim* getAnim(void) const;
	X_INLINE int32_t getCycleCount(void) const;
	X_INLINE float getStartWeight(void) const;
	X_INLINE float getFinalWeight(void) const;
	X_INLINE float getRate(void) const;
	X_INLINE core::TimeVal getBlendStart(void) const;
	X_INLINE core::TimeVal getBlendDuration(void) const;
	X_INLINE core::TimeVal getStartTime(void) const;
	X_INLINE core::TimeVal getEndTime(void) const;
	core::TimeVal getPlayTime(void) const;

private:

private:
	const Anim* pAnim_;

	core::TimeVal startTime_;
	core::TimeVal endTime_;
	core::TimeVal timeOffset_;

	core::TimeVal blendStart_;
	core::TimeVal blendDuration_;

	//  -1: loop
	//  2: play twice.
	int32_t cycles_;

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
	ANIMLIB_EXPORT explicit Animator(core::MemoryArenaBase* arena);
	ANIMLIB_EXPORT Animator(const model::XModel* pModel, core::MemoryArenaBase* arena);

	ANIMLIB_EXPORT void setModel(const model::XModel* pModel);

	X_INLINE const Mat44Arr& getBoneMatrices(void) const;
	
	ANIMLIB_EXPORT bool createFrame(core::TimeVal currentTime);

	ANIMLIB_EXPORT void clearAnims(core::TimeVal curTime, core::TimeVal clearTime);
	ANIMLIB_EXPORT void clearAnim(int32_t animNum, core::TimeVal curTime, core::TimeVal clearTime);
	ANIMLIB_EXPORT void playAnim(const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime);
	ANIMLIB_EXPORT void playAnim(const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime, core::TimeVal playTime);
	ANIMLIB_EXPORT bool isAnimating(core::TimeVal currentTime) const;

	ANIMLIB_EXPORT void getDelta(core::TimeVal fromTime, core::TimeVal toTime, Vec3f& delta) const;
	ANIMLIB_EXPORT void getOrigin(core::TimeVal currentTime, Vec3f& pos) const;

	// bones.
	ANIMLIB_EXPORT model::BoneHandle getBoneHandle(const char* pName) const;
	ANIMLIB_EXPORT bool getBoneTransform(model::BoneHandle handle, core::TimeVal currentTime, Vec3f& pos, Matrix33f& axis);


	ANIMLIB_EXPORT void renderInfo(core::TimeVal currentTime, const Vec3f& pos, const Matrix33f& mat, engine::IPrimativeContext* pPrimContex) const;


private:
	void pushAnims(core::TimeVal currentTime, core::TimeVal blendTime);

private:
	core::MemoryArenaBase* arena_;
	const model::XModel* pModel_;

	core::TimeVal lastTransformTime_;

	AnimBlendArr anims_;
	Mat44Arr boneMat_;
};


X_NAMESPACE_END

#include "Animator.inl"