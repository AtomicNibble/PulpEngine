#pragma once

#include "anim_inter.h"
#include "../ModelLib/ModelSkeleton.h"

X_NAMESPACE_BEGIN(anim)

class AnimCompiler
{
	struct Stats
	{
		Stats(core::MemoryArenaBase* arena);

		void clear(void);
		void print(void) const;

	public: 
		float scale;
		int32_t numFrames;
		int32_t fps;
		size_t totalBones;
		size_t totalBonesPosData;
		size_t totalBonesAngleData;

		core::TimeVal compileTime;
		core::Array<core::string> droppedBoneNames;
	};

	// we want to know what frames have changed data.
	class Position
	{
		struct PosFrame
		{
			int32_t frame;
			Vec3f worldPos;
		};

		typedef Vec3<uint16_t> Scaler;
		typedef core::Array<PosFrame> PosFrameArr;
		typedef core::Array<Vec3f> PosData;
		typedef core::Array<Scaler> ScalerArr;
	public:
		Position(core::MemoryArenaBase* arena);

		void save(core::ByteStream& stream) const;
		
		void appendFullPos(const Vec3f& pos);
		void setBasePosition(const Vec3f& basePos);

		size_t numPosFrames(void) const;
		bool hasData(void) const;
		bool isFullFrames(void) const;
		bool isLargeFrames(void) const;
		bool isLargeScalers(void) const;
		Vec3f minRelative(void) const;
		const Vec3f& min(void) const;
		const Vec3f& range(void) const;
		const Vec3f& basePos(void) const;
		const Vec3f& getWorldPosForFrame(size_t idx) const;


		void calculateRelativeData(const Position& parentPos);
		void calculateDeltas(const float posError = 0.075f);

	private:
		void buildScalers(const float posError);

	private:
		PosData fullPos_;
		PosData relPos_;
		PosFrameArr posFrames_;

		ScalerArr scalers_;

		Vec3f min_;
		Vec3f max_;
		Vec3f range_;
		Vec3f basePosition_;

		bool largeScalers_;
		bool _pad[3];
	};


	class Angle
	{
		struct AngleFrame
		{
			int32_t frame;
			Quatf angle;
		};

		typedef core::Array<Quatf> AnglesArr;
		typedef core::Array<AngleFrame> AngleFrameArr;

	public:
		Angle(core::MemoryArenaBase* arena);

		void save(core::ByteStream& stream) const;
		
		void appendFullAng(const Quatf& ang);
		void setBaseOrient(const Quatf& ang);
		size_t numAngleFrames(void) const;
		bool hasData(void) const;
		bool isLargeFrames(void) const;
		bool isFullFrames(void) const; 

		void calculateDeltas(const float angError = 0.075f);

	private:
		AnglesArr fullAngles_; // angles for every frame
		AngleFrameArr angles_; // the set of angles after dropping angles below angle threshold.

		Vec3<bool> axisChanges_;

		Quatf baseOrient_;
	};

	class Bone
	{
	public:
		typedef core::Array<Quatf> AngleData;

	public:
		Bone(core::MemoryArenaBase* arena);

		bool hasData(void) const;

		core::string name;
		Position pos;
		Angle ang;
		int32_t parentIdx;
	};

	typedef core::Array<Bone> BoneArr;


	X_DECLARE_FLAGS(CompileFlag)(
		LOOPING
	);

public:
	static const float DEFAULT_POS_ERRR;
	static const float DEFAULT_ANGLE_ERRR;

public:
	AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton);
	~AnimCompiler();

	void printStats(bool verbose = false) const;

	void setScale(float scale);
	void setLooping(bool loop);
	void setAnimType(AnimType::Enum type);

	bool compile(const core::Path<char>& path, const float posError = DEFAULT_POS_ERRR, const float angError = DEFAULT_ANGLE_ERRR);
	bool compile(const core::Path<wchar_t>& path, const float posError = DEFAULT_POS_ERRR, const float angError = DEFAULT_ANGLE_ERRR);

private:
	bool save(const core::Path<wchar_t>& path);

private:

	void loadInterBones(void);
	void dropMissingBones(void);
	void dropNullBones(void);
	void loadBaseData(void);
	void processBones(const float posError, const float angError);

private:
	core::MemoryArenaBase* arena_;

	const InterAnim& inter_;
	const model::ModelSkeleton& skelton_;

private:
	float scale_;
	Flags<CompileFlag> flags_;
	AnimType::Enum type_;
	BoneArr bones_;

	Stats stats_;
};


X_NAMESPACE_END