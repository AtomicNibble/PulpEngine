#include "stdafx.h"

#if 0

#include "AnimInstance.h"
#include "Anim.h"

X_NAMESPACE_BEGIN(anim)

AnimInstance::AnimInstance(const Anim& anim, core::MemoryArenaBase* arena) :
	anim_(anim),
	arena_(arena),
	boneMat_(arena, anim.numBones())
{

}

void AnimInstance::update(core::TimeVal delta)
{
	// update our bones for given time.
	anim_.update(delta, state_, boneMat_);
}

const AnimInstance::Mat44Arr& AnimInstance::getBoneMats(void) const
{
	return boneMat_;
}



X_NAMESPACE_END

#endif