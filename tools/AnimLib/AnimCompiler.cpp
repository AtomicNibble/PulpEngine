#include "stdafx.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)


AnimCompiler::AnimCompiler(const InterAnim& inter, const model::ModelSkeleton& skelton) :
	inter_(inter),
	skelton_(skelton)
{

}


AnimCompiler::~AnimCompiler()
{

}


X_NAMESPACE_END