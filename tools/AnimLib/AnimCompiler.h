#pragma once

#include "anim_inter.h"
#include "ModelSkeleton.h"

X_NAMESPACE_BEGIN(anim)


class AnimCompiler
{

public:
	AnimCompiler(const InterAnim& inter, const model::ModelSkeleton& skelton);
	~AnimCompiler();


private:
	const InterAnim& inter_;
	const model::ModelSkeleton& skelton_;
};


X_NAMESPACE_END