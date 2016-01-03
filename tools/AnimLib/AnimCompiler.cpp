#include "stdafx.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)



AnimCompiler::Position::Position(core::MemoryArenaBase* arena) :
	fullPos(arena),
	posDeltas(arena),
	scalers(arena)
{
	largeScalers = false;
}


void AnimCompiler::Position::CalculateDeltas(const float posError)
{
	posDeltas.clear();
	posDeltas.reserve(fullPos.size());

	// we only store a delta if it's diffrent.
	int32_t frame = 0;
	Vec3f curPos = basePosition;

	min = Vec3f::max();
	max = Vec3f::min();

	for (auto& pos : fullPos)
	{
		Vec3f delta = curPos - pos;

		if (!delta.compare(Vec3f::zero(), posError))
		{
			// only store min max for positions we are going to move to.
			min.checkMin(pos);
			max.checkMax(pos);

			// we moved enougth.
			PosDelta& deltaEntry = posDeltas.AddOne();
			deltaEntry.worldPos = pos;
			deltaEntry.delta = delta;
			deltaEntry.frame = frame;

			curPos = pos;
		}

		frame++;
	}

	BuildScalers(posError);
}


void AnimCompiler::Position::BuildScalers(const float posError)
{
	range = max - min;

	uint32_t segments = (1 << 8) - 1;

	// work out if we can use 8bit scalers with selected posError.
	Vec3f rangePercision = range / 255;
	if (rangePercision.x > posError || rangePercision.y > posError || rangePercision.z > posError) {
		segments = (1 << 16) - 1; // need 16 bit.

		largeScalers = true;
	}
	else {
		largeScalers = false;
	}

	Vec3f segmentsize(range / Vec3f(static_cast<float>(segments)));

	scalers.clear();
	scalers.reserve(posDeltas.size());

	for (auto& deltaEntry : posDeltas)
	{
		// we know want to know the scaler needed to get the world pos.
		Vec3f worldDelta = deltaEntry.worldPos - min;
		Vec3f scalerF = (worldDelta / segmentsize);
	

		Scaler scaler;
		scaler.x = static_cast<Scaler::value_type>(math<float>::round(scalerF.x));
		scaler.y = static_cast<Scaler::value_type>(math<float>::round(scalerF.y));
		scaler.z = static_cast<Scaler::value_type>(math<float>::round(scalerF.z));

		scalers.append(scaler);
	}
}


// ====================================================


AnimCompiler::Angle::Angle(core::MemoryArenaBase* arena) :
	fullAngles(arena),
	angles(arena)
{

}

void AnimCompiler::Angle::CalculateDeltas(const float posError)
{
	angles.clear();
	angles.reserve(fullAngles.size());

	Quatf curAng = baseOrient;
	int32_t frame = 0;

	for (auto& ang : fullAngles)
	{
		Quatf delta = curAng * ang.inverse();

		float pitchDelta = delta.getPitch();
		float rollDelta = delta.getRoll();
		float yawDelta = delta.getYaw();

		bool pitchPass = math<float>::abs(pitchDelta) > posError;
		bool rollPass = math<float>::abs(rollDelta) > posError;
		bool yawPass = math<float>::abs(yawDelta) > posError;

		if (pitchPass || rollPass || yawPass)
		{
			axisChanges[0] |= pitchPass;
			axisChanges[1] |= rollPass;
			axisChanges[2] |= yawPass;

			// add it :)
			AngleFrame a;
			a.frame = frame;
			a.angle = ang;

			angles.append(a);
		}

		frame++;
	}
}

// ====================================================

AnimCompiler::Bone::Bone() : 
	pos(g_AnimLibArena),
	ang(g_AnimLibArena)
{

}

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

	if (inter_.getFps() > anim::ANIM_MAX_FPS) {
		X_ERROR("Anim", "inter anim fps exceeds max: %i", 
			anim::ANIM_MAX_FPS);
		return false;
	}
	if (inter_.getNumFrames() > anim::ANIM_MAX_FRAMES) {
		X_ERROR("Anim", "inter anim exceeds max frames: %i",
			anim::ANIM_MAX_FRAMES);
		return false;
	}


	loadInterBones();
	dropMissingBones();

	if (bones_.isEmpty()) {
		X_WARNING("Anim", "skipping compile of anim, inter anim and model skelton have bones in common");
		return true;
	}

	// load base bone positions from model skelton.
	loadBaseData();
	processBones();

	return true;
}


void AnimCompiler::loadInterBones(void)
{
	// make a copy of all the bones names in anim file.
	bones_.resize(inter_.getNumBones());

	for (size_t i = 0; i < inter_.getNumBones(); i++)
	{
		const anim::Bone& interBone = inter_.getBone(i);
		const size_t dataNum = interBone.data.size();

		bones_[i].name = interBone.name;
		bones_[i].pos.fullPos.reserve(dataNum);
		bones_[i].ang.fullAngles.reserve(dataNum);

		// load pos / angles.
		for (size_t x = 0; x < dataNum; x++)
		{
			bones_[i].pos.fullPos.append(interBone.data[x].position);
			bones_[i].ang.fullAngles.append(interBone.data[x].rotation);
		}
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


void AnimCompiler::loadBaseData(void)
{
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
				const XQuatCompressedf& angle =	skelton_.getBoneAngle(x);
				const Vec3f& pos = skelton_.getBonePos(x);

				Bone& bone = bones_[i];
				bone.pos.basePosition = pos;
				bone.ang.baseOrient = angle.asQuat();
				break;
			}
		}

		if (x == numModelBones)
		{
			X_ASSERT_UNREACHABLE();
		}
	}
}


void AnimCompiler::processBones(void)
{
	for (auto bone : bones_)
	{
		bone.pos.CalculateDeltas();
		bone.ang.CalculateDeltas();
	}
}

X_NAMESPACE_END