#include "stdafx.h"
#include "Animator.h"

#include <Time\TimeLiterals.h>

#include "Anim.h"

#include <../../tools/ModelLib/ModelLib.h>

X_NAMESPACE_BEGIN(anim)


AnimBlend::AnimBlend(core::MemoryArenaBase* arena) :
	indexMap_(arena)
{
	pAnim_ = nullptr;

	blendStartVal_ = 0.f;
	blendEndVal_ = 0.f;
	rate_ = 1.0f;
}


void AnimBlend::clear(void)
{
	pAnim_ = nullptr;

	blendStartVal_ = 0.f;
	blendEndVal_ = 0.f;
	rate_ = 0.001f; // 1.0f;

	startTime_ = 0_tv;
	endTime_ = 0_tv;

	blendStart_ = 0_tv;
	blendDuration_ = 0_tv;

	indexMap_.clear();
}

void AnimBlend::clear(core::TimeVal currentTime, core::TimeVal clearTime)
{
	if (clearTime == 0_tv) {
		clear();
		return;
	}


}

void AnimBlend::playAnim(const model::XModel& model, const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime)
{
	clear();

	pAnim_ = pAnim;

	const core::TimeVal oneSecond = 1000_ms;

	core::TimeVal length = core::TimeVal((pAnim->getNumFrames() * oneSecond.GetValue()) / pAnim->getFps());

	startTime_ = startTime;
	endTime_ = startTime + length;


	blendStart_ = startTime;
	blendDuration_ = blendTime;
	blendStartVal_ = 0.f;
	blendEndVal_ = 1.f;

	// build the index map.
	indexMap_.resize(model.numBones());

	// for every bone in the model we find the bone in animation.
	// we allow for multiple animations to be played that affect a subset of bones.
	// we will pass the bones to the animation.
	// and the anim needs to know which model bones it effects
	// every index that is -1 the anim don't effect
	// others are indexes of the animation's bones.
	for (int32_t i = 0; i < model.numBones(); i++)
	{
		const char* pBoneName = model.getBoneName(i);

		indexMap_[i] = -1;

		// find this in the animation.
		for (int32_t j = 0; j < pAnim->getNumBones(); j++)
		{
			const char* pAnimBoneName = pAnim->getBoneName(j);

			if (core::strUtil::IsEqual(pBoneName, pAnimBoneName))
			{
				indexMap_[i] = j;
				break;
			}
		}

	}
}

bool AnimBlend::blend(core::TimeVal currentTime, TransformArr& boneTransOut, float &blendWeight) const
{
	if (!pAnim_) {
		return false;
	}

	float weight = getWeight(currentTime);

	if (blendWeight > 0.0f) 
	{
		if (endTime_ >= 0_tv && currentTime >= endTime_) {
			return false;
		}
	
		if (!weight) {
			return false;
		}
	}

	auto time = animTime(currentTime);

	X_ASSERT(time >= 0_tv, "Negative time value")(time);

	// so we support applying animations that don't effect all bones.
	// so we are given the models bones and need to update the correct indexes.
	// the animation might also have more bones than the model as we need to just ignore them.
	// can i do that with a single index map or need pairs?
	// i need a list of bones we effect in the model and the indexes of them.
	// but also need the index of the animation bone.
	// maybe just have a list that is size of model bones, but with -1 if we don't animate it.

	FrameBlend frame;
	pAnim_->timeToFrame(time, frame);
	pAnim_->getFrame(frame, boneTransOut, indexMap_);

	if (!blendWeight)
	{
		blendWeight = weight;
	}
	else 
	{
		// need to blend in bones.
		blendWeight += weight;
		float lerp = weight / blendWeight;
		// Util::blendBones(blendFrame, jointFrame, lerp);
	}


	return true;
}


bool AnimBlend::isDone(core::TimeVal currentTime) const
{
	if (endTime_ > 0_tv && (currentTime >= endTime_)) {
		return true;
	}

	if (blendEndVal_ <= 0.0f && (currentTime >= (blendStart_ + blendDuration_))) {
		return true;
	}

	return false;
}

float AnimBlend::getWeight(core::TimeVal currentTime) const
{
	core::TimeVal timeDelta = currentTime - blendStart_;
	if (timeDelta <= 0_tv) {
		return blendStartVal_;
	}
	else if (timeDelta >= blendDuration_) {
		return blendEndVal_;
	}
	
	auto frac = timeDelta.GetMilliSeconds() / blendDuration_.GetMilliSeconds();
	return blendStartVal_ + (blendEndVal_ - blendStartVal_) * frac;
}


core::TimeVal AnimBlend::animTime(core::TimeVal currentTime) const
{
	if (!pAnim_) {
		return 0_tv;
	}

	if (rate_ == 1.f)
	{
		return currentTime - startTime_;
	}

	X_ASSERT(rate_ >= 0.f, "Invalid rate")(rate_);

	float elapsedMS = (currentTime - startTime_).GetMilliSeconds();
	float scaled = elapsedMS * rate_;

	return core::TimeVal(scaled);
}

// ------------------------------

Animator::Animator(const model::XModel& model, core::MemoryArenaBase* arena) :
	arena_(arena),
	model_(model),
	boneMat_(arena), 
	anims_{ {
		X_PP_REPEAT_COMMA_SEP(2, arena)
	}}
{
	boneMat_.resize(model.numBones());
}


bool Animator::createFrame(core::TimeVal currentTime)
{
	if (lastTransformTime_ == currentTime) {
		return false;
	}

	lastTransformTime_ = currentTime;

	// joints for whole model.
	TransformArr bones(g_AnimLibArena, model_.numBones());

	// YER BOI.
	float weight = 0.f;
	bool aniamted = false;

	for (auto& anim : anims_) {
		if (anim.blend(currentTime, bones, weight)) {
			aniamted = true;
			if (weight >= 1.0f) {
				break;
			}
		}
	}

	if (!aniamted) {
		return false;
	}

	Util::convertBoneTransToMatrix(boneMat_, bones);
	return true;
}

void Animator::playAnim(const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime)
{
	// push anims down one.
	pushAnims(startTime, blendTime);

	anims_[0].playAnim(model_, pAnim, startTime, blendTime);
}

void Animator::pushAnims(core::TimeVal currentTime, core::TimeVal blendTime) 
{
	if (!anims_[0].getWeight(currentTime) || anims_[0].getStartTime() == currentTime) {
		return;
	}

	// shift them down.
	std::rotate(anims_.begin(), anims_.begin() + 1, anims_.end());

	// clear first anim.
	anims_[0].clear();
	anims_[1].clear(currentTime, blendTime);
}

bool Animator::isAnimating(core::TimeVal currentTime) const
{
	for (auto& anim : anims_)
	{
		if (!anim.isDone(currentTime))
		{
			return true;
		}
	}

	return false;
}


X_NAMESPACE_END