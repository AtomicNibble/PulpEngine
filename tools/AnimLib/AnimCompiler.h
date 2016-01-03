#pragma once

#include "anim_inter.h"
#include "ModelSkeleton.h"

X_NAMESPACE_BEGIN(anim)

class AnimCompiler
{
	struct Bone
	{
		core::string name;
	};

	typedef core::Array<Bone> BoneArr;

public:
	AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton);
	~AnimCompiler();

	bool compile(void);

private:

	void loadInterBoneNames(void);
	void dropMissingBones(void);

private:
	const InterAnim& inter_;
	const model::ModelSkeleton& skelton_;
private:

	BoneArr bones_;
};


X_NAMESPACE_END