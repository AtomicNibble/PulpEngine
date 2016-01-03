#pragma once

#include "anim_inter.h"
#include "ModelSkeleton.h"

X_NAMESPACE_BEGIN(anim)

class AnimCompiler
{
	// we want to know what frames have changed data.

	struct PosDelta
	{
		int32_t frame;
		Vec3f worldPos;
		Vec3f delta;
	};

	struct Position
	{
		typedef Vec3<uint16_t> Scaler;
		typedef core::Array<PosDelta> PosDeltaArr;
		typedef core::Array<Vec3f> PosData;
		typedef core::Array<Scaler> ScalerArr;

		Position(core::MemoryArenaBase* arena);

		void CalculateDeltas(const float posError = 0.075f);
	private:
		void BuildScalers(const float posError);

	public:
		PosData fullPos;
		PosDeltaArr posDeltas;

		ScalerArr scalers;

		Vec3f min;
		Vec3f max;
		Vec3f range;
		Vec3f basePosition;

		bool largeScalers;
		bool _pad[3];
	};

	struct AngleFrame
	{
		int32_t frame;
		Quatf angle;

	};

	struct Angle
	{
		typedef core::Array<Quatf> AnglesArr;
		typedef core::Array<AngleFrame> AngleFrameArr;

		Angle(core::MemoryArenaBase* arena);

		void CalculateDeltas(const float posError = 0.075f);

	public:
		AnglesArr fullAngles;
		AngleFrameArr angles;

		Vec3<bool> axisChanges;

		Quatf baseOrient;
	};

	struct Bone
	{

		typedef core::Array<Quatf> AngleData;

		Bone();

		core::string name;

		Position pos;
		Angle ang;
	//	AngleData angles;
	//	Quatf baseOrent;
	};

	typedef core::Array<Bone> BoneArr;

public:
	AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton);
	~AnimCompiler();

	bool compile(void);

private:

	void loadInterBones(void);
	void dropMissingBones(void);
	void loadBaseData(void);
	void processBones(void);

private:
	const InterAnim& inter_;
	const model::ModelSkeleton& skelton_;
private:

	BoneArr bones_;
};


X_NAMESPACE_END