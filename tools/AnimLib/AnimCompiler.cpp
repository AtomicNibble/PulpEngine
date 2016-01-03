#include "stdafx.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)


AnimCompiler::AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton) :
	inter_(inter),
	skelton_(skelton),

	bones_(arena)
{

}


AnimCompiler::~AnimCompiler()
{

}

bool AnimCompiler::compile(void)
{
	// got any bones in the inter?
	if (inter_.getNumBones() < 1) {
		X_WARNING("Anim", "skipping compile of anim, source inter anim has no bones");
		return false; 
	}
	// any anims in the model skeleton?
	if (skelton_.getNumBones() < 1) {
		X_WARNING("Anim", "skipping compile of anim, source model skeleton has no bones");
		return false;
	}


	loadInterBoneNames();
	dropMissingBones();

	if (bones_.isEmpty()) {
		X_WARNING("Anim", "skipping compile of anim, inter anim and model skelton have bones in common");
		return true;
	}



	return false;
}


void AnimCompiler::loadInterBoneNames(void)
{
	// make a copy of all the bones names in anim file.
	bones_.resize(inter_.getNumBones());

	for (size_t i = 0; i < inter_.getNumBones(); i++)
	{
		const anim::Bone& interBone = inter_.getBone(i);
		bones_[i].name = interBone.name;
	}
}

void AnimCompiler::dropMissingBones(void)
{
	// drop any bones that are not in the model file.
	size_t i;
	for (i = 0; i < bones_.size(); i++)
	{
		const core::string& name = bones_[i].name;

		// check if this bone in model file.
		size_t x, numModelBones = skelton_.getNumBones();
		for (x = 0; x < numModelBones; x++)
		{
			const char* pName = skelton_.getBoneName(x);
			if (name == pName)
			{
				break;
			}
		}

		if (x == numModelBones)
		{
			// remove it.
			bones_.removeIndex(i);
		}
	}
}

X_NAMESPACE_END