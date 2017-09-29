#include "stdafx.h"
#include "Animator.h"

#include <Time\TimeLiterals.h>
#include <IPrimativeContext.h>
#include <IFont.h>

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

	startTime_ = 0_tv;
	endTime_ = 0_tv;

	blendStart_ = 0_tv;
	blendDuration_ = 0_tv;

	cycles_ = 1;

	blendStartVal_ = 0.f;
	blendEndVal_ = 0.f;
	rate_ = 1.0f;

	indexMap_.clear();
}

void AnimBlend::clear(core::TimeVal currentTime, core::TimeVal clearTime)
{
	if (clearTime == 0_tv) {
		clear();
		return;
	}

	// blend out.
	setWeight(0.f, currentTime, clearTime);
}

void AnimBlend::playAnim(const model::XModel& model, const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime)
{
	clear();

	pAnim_ = pAnim;

	startTime_ = startTime;
	endTime_ = startTime + pAnim->getDuration();

	blendStart_ = startTime;
	blendDuration_ = blendTime;
	blendStartVal_ = 0.f;
	blendEndVal_ = 1.f;

	if (pAnim->isLooping()) {
		setCycleCount(-1);
	}
 
	// build the index map.
	indexMap_.resize(model.getNumBones());

	// for every bone in the model we find the bone in animation.
	// we allow for multiple animations to be played that affect a subset of bones.
	// we will pass the bones to the animation.
	// and the anim needs to know which model bones it effects
	// every index that is -1 the anim don't effect
	// others are indexes of the animation's bones.
	for (int32_t i = 0; i < model.getNumBones(); i++)
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

	TransformArr tempTrans(boneTransOut.getArena());
	TransformArr* pTransArr = &boneTransOut;

	// animate into a temp buffer.
	if (blendWeight > 0.f)
	{
		tempTrans.resize(boneTransOut.size());
		pTransArr = &tempTrans;
	}

	FrameBlend frame;
	pAnim_->timeToFrame(time, cycles_, frame);
	pAnim_->getFrame(frame, *pTransArr, indexMap_);

	if (!blendWeight)
	{
		blendWeight = weight;
	}
	else 
	{
		blendWeight += weight;
		float lerp = weight / blendWeight;
		Util::blendBones(boneTransOut, *pTransArr, indexMap_, lerp);
	}

	return true;
}

void AnimBlend::blendOrigin(core::TimeVal currentTime, Vec3f& blendPos, float& blendWeight) const
{
	if (endTime_ > 0_tv && (currentTime > endTime_)) {
		return;
	}
	
	float weight = getWeight(currentTime);
	if (!weight) {
		return;
	}
	
	auto time = animTime(currentTime);
	Vec3f pos;

	pAnim_->getOrigin(pos, currentTime, cycles_);

	pos = pos * weight;

	if (!blendWeight) {
		blendPos = pos;
		blendWeight = weight;
	}
	else {
		float fract = weight / (blendWeight + weight);
		blendPos = blendPos.lerp(fract, pos); 
		blendWeight += weight;
	}
}

void AnimBlend::blendDelta(core::TimeVal fromTime, core::TimeVal toTime, Vec3f& blendDelta, float& blendWeight) const
{
	if (endTime_ > 0_tv && (fromTime > endTime_)) {
		return;
	}

	float weight = getWeight(toTime);
	if (!weight) {
		return;
	}

	auto time1 = animTime(fromTime);
	auto time2 = animTime(toTime);
	if (time2 < time1) {
		time2 += pAnim_->getDuration();
	}

	Vec3f pos1, pos2;

	pAnim_->getOrigin(pos1, fromTime, cycles_);
	pAnim_->getOrigin(pos1, toTime, cycles_);

	pos1 = (pos1 * weight);
	pos1 = (pos2 * weight);
	Vec3f delta = pos2 - pos1;

	if (!blendWeight) {
		blendDelta = delta;
		blendWeight = weight;
	}
	else {
		float fract = weight / (blendWeight + weight);
		blendDelta = blendDelta.lerp(fract, delta);
		blendWeight += weight;
	}
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

	auto elapsed = (currentTime - startTime_).GetValue();
	auto scaled = static_cast<core::TimeVal::TimeType>(static_cast<float>(elapsed) * rate_);

	return core::TimeVal(scaled);
}


void AnimBlend::setCycleCount(int32_t numCycles)
{
	cycles_ = numCycles;

	auto duration = pAnim_->getDuration();

	if (cycles_ < 0)
	{
		cycles_ = -1;
		endTime_ = 0_tv;
	}
	else if (cycles_ == 0)
	{
		cycles_ = 1;

		if (rate_ == 1.0f) 
		{
			endTime_ = startTime_ - timeOffset_ + duration;
		}
		else
		{
			core::TimeVal scaledDuration(static_cast<core::TimeVal::TimeType>(static_cast<float>(duration.GetMilliSeconds()) * rate_));
			endTime_ = startTime_ - timeOffset_ + scaledDuration;
		}
	}
	else
	{
		duration = core::TimeVal(duration.GetValue() * cycles_);

		if (rate_ == 1.0f)
		{
			endTime_ = startTime_ - timeOffset_ + duration;
		}
		else
		{
			core::TimeVal scaledDuration(static_cast<core::TimeVal::TimeType>(
				static_cast<float>(duration.GetMilliSeconds()) * rate_));
			
			endTime_ = startTime_ - timeOffset_ + scaledDuration;
		}
	}
}

void AnimBlend::setRate(float rate)
{
	X_ASSERT(rate >= 0.f, "Invalid rate")(rate);

	if (rate_ == rate) {
		return;
	}

	rate_ = rate;

	// force a update of end time
	setCycleCount(cycles_);
}

void AnimBlend::setWeight(float newWeight, core::TimeVal currentTime, core::TimeVal blendTime)
{
	blendStartVal_ = getWeight(currentTime);
	blendEndVal_ = newWeight;

	blendStart_ = currentTime;
	blendDuration_ = blendTime;

	if (!newWeight) {
		endTime_ = currentTime + blendTime;
	}
}

core::TimeVal AnimBlend::getPlayTime(void) const
{
	if (endTime_ == 0_tv) {
		return 0_tv;
	}

	return endTime_ - startTime_;
}

// ------------------------------


Animator::Animator(core::MemoryArenaBase* arena) :
	Animator(nullptr, arena)
{
}


Animator::Animator(const model::XModel* pModel, core::MemoryArenaBase* arena) :
	arena_(arena),
	pModel_(pModel),
	boneMat_(arena),
	anims_{ {
		X_PP_REPEAT_COMMA_SEP(2, arena)
	}}
{
	setModel(pModel);
}

void Animator::setModel(const model::XModel* pModel)
{
	pModel_ = pModel;

	if (pModel) {
		boneMat_.resize(pModel->getNumBones());
	}
	else {
		boneMat_.clear();
	}
}


bool Animator::createFrame(core::TimeVal currentTime)
{
	if (!pModel_) {
		return false;
	}

	// dunno how i want to deal with this, just return for now.
	if (pModel_->getNumBones() == pModel_->getNumRootBones()) {
		return false;
	}

	if (lastTransformTime_ == currentTime) {
		return false;
	}

	if (!isAnimating(currentTime)) {
		return false;
	}
	
	lastTransformTime_ = currentTime;

	// joints for whole model.
	TransformArr bones(g_AnimLibArena, pModel_->getNumBones());

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

	{
		auto angle = pModel_->getBoneAngle(0);
		auto pos = pModel_->getBonePos(0);

		bones[0].pos = pos;
		bones[0].quat = angle.asQuat();
	}

	// move the relative transforms into bone space.
	for (size_t i = 0; i < bones.size(); i++)
	{
		bones[i].pos += pModel_->getBonePosRel(i);
	}

	Util::convertBoneTransToMatrix(boneMat_, bones);
	Util::transformBones(boneMat_, pModel_->getTagTree(), 1, static_cast<int32_t>(boneMat_.size() - 1));

	// the boneMat_ are now all in model space.

	return true;
}

void Animator::clearAnims(core::TimeVal curTime, core::TimeVal clearTime)
{
	for (auto& anim : anims_)
	{
		anim.clear(curTime, clearTime);
	}
}

void Animator::clearAnim(int32_t animNum, core::TimeVal curTime, core::TimeVal clearTime)
{
	anims_[animNum].clear(curTime, clearTime);
}

void Animator::playAnim(const Anim* pAnim, core::TimeVal startTime, core::TimeVal blendTime)
{
	if (!pModel_) {
		return;
	}

	// push anims down one.
	pushAnims(startTime, blendTime);

	anims_[0].playAnim(*pModel_, pAnim, startTime, blendTime);
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
	if (!pModel_) {
		return false;
	}

	for (auto& anim : anims_)
	{
		if (!anim.isDone(currentTime))
		{
			return true;
		}
	}

	return false;
}

void Animator::getDelta(core::TimeVal fromTime, core::TimeVal toTime, Vec3f& delta) const
{
	delta = Vec3f::zero();
	if (fromTime == toTime) {
		return;
	}

	float weight = 0.f;

	for (auto& anim : anims_)
	{
		anim.blendDelta(fromTime, toTime, delta, weight);
	}
}

void Animator::getOrigin(core::TimeVal currentTime, Vec3f& pos) const
{
	float weight = 0.f;

	for (auto& anim : anims_)
	{
		anim.blendOrigin(currentTime, pos, weight);
	}
}


model::BoneHandle Animator::getBoneHandle(const char* pName) const
{
	return pModel_->getBoneHandle(pName);
}

bool Animator::getBoneTransform(model::BoneHandle handle, core::TimeVal currentTime, Vec3f& pos, Matrix33f& axis)
{
	if (!pModel_) {
		return false;
	}

	if (handle == model::INVALID_BONE_HANDLE) {
		return false;
	}

	X_ASSERT(handle < pModel_->getNumBones(), "Out of range")(handle, pModel_->getNumBones());

	// create a frame if needed,
	createFrame(currentTime);

	pos = boneMat_[handle].getTranslate().xyz();
	axis = boneMat_[handle].subMatrix33(0,0);
	return true;
}


void Animator::renderInfo(core::TimeVal currentTime, const Vec3f& pos, const Matrix33f& mat, engine::IPrimativeContext* pPrimContex) const
{
	core::StackString512 txt;

	for (auto& anim : anims_)
	{
		auto* pAnim = anim.getAnim();
		if (!pAnim) {
			continue;
		}

		auto animTime = anim.animTime(currentTime);
		bool isDone = anim.isDone(currentTime);
		
		if (isDone) {
			animTime = anim.animTime(anim.getEndTime());
		}

		auto dur = pAnim->getDuration();
		auto start = anim.getStartTime();
		auto end = anim.getEndTime();
		auto play = anim.getPlayTime();

		auto durSec = dur.GetSeconds();
		auto animTimeSec = animTime.GetSeconds();
		auto startSec = start.GetSeconds();
		auto endSec = end.GetSeconds();
		auto playSec = play.GetSeconds();

		txt.appendFmt("Name: %s\n", pAnim->getName());
		txt.appendFmt("Frames: %" PRIi32 " Fps: %" PRIi32 " rate: %.2f\n", pAnim->getNumFrames(), pAnim->getFps(), anim.getRate());
		txt.appendFmt("Dur: %.2fs AnimTime: %.2fs\n", durSec, animTimeSec);
		txt.appendFmt("Start: %.2fs End: %.2fs Play: %.2fs\n", startSec, endSec, playSec);
		txt.appendFmt("Weights: start: %.2f final: %.2f cur: %.2f \n", anim.getStartWeight(), anim.getFinalWeight(), anim.getWeight(currentTime));
		txt.appendFmt("NumCycles: %" PRIi32 " isDone: %i\n", anim.getCycleCount(), anim.isDone(currentTime));

		{
			FrameBlend frame;
			pAnim->timeToFrame(animTime, anim.getCycleCount(), frame);

			txt.appendFmt("Blend: f1: %i f2: %i cycles: %i\n", frame.frame1, frame.frame2, frame.cylces);
			txt.appendFmt("Blend: f-lerp: %.2f b-lerp: %.2f\n", frame.frontlerp, frame.backlerp);
		}
	}


	font::TextDrawContext ctx;
	ctx.col = Col_White;
	ctx.size = Vec2f(3.f, 3.f);
	ctx.effectId = 0;
	ctx.pFont = gEnv->pFontSys->GetDefault();
	ctx.flags.Set(font::DrawTextFlag::FRAMED);

	pPrimContex->drawText(pos, mat, ctx, txt.begin(), txt.end());

}




X_NAMESPACE_END