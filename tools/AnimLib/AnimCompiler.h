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
		core::Array<core::string> noneAnimatedBoneName;
		core::Array<core::string> notPresentBoneNames;
	};

	class Angle;

	// we want to know what frames have changed data.
	class Position
	{
		struct PosFrame
		{
			int32_t frame;
			Vec3f relPos;
			Vec3f delta;
		};

		typedef Vec3<uint16_t> Scaler;
		typedef core::Array<PosFrame> PosFrameArr;
		typedef core::Array<Vec3f> PosData;
		typedef core::Array<Scaler> ScalerArr;
	public:
		Position(core::MemoryArenaBase* arena);

		void save(core::ByteStream& stream) const;
		void clearData(void);

		void appendFullPos(const Vec3f& pos);
		void setBasePositions(const Vec3f& basePosWorld, const Vec3f& basePosRel);

		size_t numPosFrames(void) const;
		bool hasData(void) const;
		bool isFullFrames(void) const;
		bool isLargeFrames(void) const;
		bool isLargeScalers(void) const;
		const Vec3f& min(void) const;
		const Vec3f& range(void) const;
		const Vec3f& basePosWorld(void) const;
		const Vec3f& basePosRel(void) const;
		const Vec3f& getWorldPosForFrame(size_t idx) const;

		const PosFrameArr& getPositions(void) const;

		void calculateRelativeData(const Position& parentPos, const Angle& parentAng);
		void calculateRelativeDataRoot(void);
		void calculateFullFrames(void);
		void calculateDeltaFrames(const float posError = 0.075f);
		void buildScalers(const float posError);

	private:
		PosData fullPos_;
		PosData relPos_;
		PosFrameArr posFrames_;

		ScalerArr scalers_;

		Vec3f min_;
		Vec3f max_;
		Vec3f range_;
		Vec3f basePosWorld_;
		Vec3f basePosRel_;

		bool largeScalers_;
		bool _pad[3];
	};


	class Angle
	{
		struct AngleFrame
		{
			int32_t frame;
			Matrix33f angle;
		};

		typedef core::Array<Matrix33f> AnglesArr;
		typedef core::Array<AngleFrame> AngleFrameArr;

	public:
		Angle(core::MemoryArenaBase* arena);

		void save(core::ByteStream& stream) const;
		void clearData(void);

		void appendFullAng(const Matrix33f& ang);
		void setBaseOrients(const Quatf& angWorld, const Quatf& andRel);

		size_t numAngleFrames(void) const;
		bool hasData(void) const;
		bool isLargeFrames(void) const;
		bool isFullFrames(void) const; 
		
		const Matrix33f& getAngForFrame(size_t idx) const;
		const AngleFrameArr& getAngles(void) const;

		void calculateRelativeData(const Position& parentPos, const Angle& parentAng);
		void calculateRelativeDataRoot(void);
		void calculateFullFrames(void);
		void calculateDeltaFrames(const float angError = 0.075f);

	private:
		AnglesArr fullAngles_; // angles for every frame
		AnglesArr relAngles_; // angles for every frame
		AngleFrameArr angles_; // the set of angles after dropping angles below angle threshold.

		Vec3<bool> axisChanges_;

		Quatf baseOrientWorld_;
		Quatf baseOrient_;
	};

	class Bone
	{
	public:
		typedef core::Array<Quatf> AngleData;

	public:
		Bone(core::MemoryArenaBase* arena);

		bool hasData(void) const;
		void clearData(void);

		core::string name;
		Position pos;
		Angle ang;
		int32_t parentIdx;
	};

	typedef core::Array<Bone> BoneArr;
	typedef InterAnim::NoteArr NoteArr;

	X_DECLARE_FLAGS(CompileFlag)(
		LOOPING,
		NO_OPTIMISE
	);

public:
	static const float DEFAULT_POS_ERRR;
	static const float DEFAULT_ANGLE_ERRR;
	static const float NOISE_ELIPSON;

public:
	AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton);
	~AnimCompiler();

	void printStats(bool verbose = false) const;

	void setScale(float scale);
	void setLooping(bool loop);
	void disableOptimizations(bool disable);
	void setAnimType(AnimType::Enum type);

	bool compile(const core::Path<char>& path, const float posError = DEFAULT_POS_ERRR, const float angError = DEFAULT_ANGLE_ERRR);
	bool compile(const core::Path<wchar_t>& path, const float posError = DEFAULT_POS_ERRR, const float angError = DEFAULT_ANGLE_ERRR);

private:
	bool save(const core::Path<wchar_t>& path);

private:
	void loadBones(void);
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
	NoteArr notes_;

	Stats stats_;
};


X_NAMESPACE_END