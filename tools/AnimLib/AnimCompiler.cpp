#include "stdafx.h"
#include "AnimCompiler.h"

#include <Time\StopWatch.h>
#include <String\HumanDuration.h>

#include <IFileSys.h>


X_NAMESPACE_BEGIN(anim)

const float AnimCompiler::DEFAULT_POS_ERRR = 0.05f;
const float AnimCompiler::DEFAULT_ANGLE_ERRR = 0.005f;

AnimCompiler::Stats::Stats(core::MemoryArenaBase* arena) :
	droppedBoneNames(arena)
{
	clear();
}

void AnimCompiler::Stats::clear(void)
{
	scale = 1.0f;
	numFrames = 0;
	fps = 0;
	totalBones = 0;
	totalBonesPosData = 0;
	totalBonesAngleData = 0;

	compileTime = core::TimeVal(0ll);
	droppedBoneNames.clear();
}

void AnimCompiler::Stats::print(void) const
{
	core::HumanDuration::Str durStr;

	X_LOG0("Anim", "Anim Info:");
	X_LOG0("Anim", "> Scale: ^6%g", scale);
	X_LOG0("Anim", "> Compile Time: ^6%s", core::HumanDuration::toString(durStr, compileTime.GetMilliSeconds()));
	X_LOG0("Anim", "> Num Frames: ^6%" PRIi32, numFrames);
	X_LOG0("Anim", "> Fps: ^6%" PRIi32, fps);
	X_LOG0("Anim", "> Total Bones: ^6%" PRIuS, totalBones);
	X_LOG0("Anim", "> Total Bones(Pos): ^6%" PRIuS, totalBonesPosData);
	X_LOG0("Anim", "> Total Bones(Ang): ^6%" PRIuS, totalBonesAngleData);

	core::StackString<1024> info("Dropped Bones:");

	if (droppedBoneNames.size() > 0) {
		info.append(" -> (");
		for (uint i = 0; i < droppedBoneNames.size(); i++) {
			info.append(droppedBoneNames[i].c_str());

			if (i < (droppedBoneNames.size() - 1)) {
				info.append(", ");
			}

			if (i > 9 && (i % 10) == 0) {
				info.append("");
			}
		}
		info.append(")");
	}
	if (droppedBoneNames.size() > 10) {
		info.append("");
	}

	X_LOG0("Anim", info.c_str());
}

// ------------------------------------------

AnimCompiler::Position::Position(core::MemoryArenaBase* arena) :
	fullPos_(arena),
	posDeltas_(arena),
	scalers_(arena)
{
	fullPos_.setGranularity(128);

	largeScalers_ = false;
}


void AnimCompiler::Position::save(core::ByteStream& stream) const
{
	int16_t numPos = safe_static_cast<uint16_t>(posDeltas_.size());

	stream.write(numPos);

	auto relativeMin = minRelative();

	if (numPos == 0)
	{
		stream.write(relativeMin); // just write min pos.
	}
	else
	{
		// if we not full frames we write frames numbers out.
 		if (!isFullFrames() && posDeltas_.size() > 1)
		{
			size_t numFrames = fullPos_.size();

			// frame numbers are 8bit if total anim frames less than 255
			if (numFrames <= std::numeric_limits<uint8_t>::max())
			{
				for (const PosDelta& delta : posDeltas_)
				{
					uint8_t frame = safe_static_cast<uint8_t, uint32_t>(delta.frame);
					stream.write(frame);
				}
			}
			else
			{
				for (const PosDelta& delta : posDeltas_)
				{
					uint16_t frame = safe_static_cast<uint16_t, uint32_t>(delta.frame);
					stream.write(frame);
				}
			}
		}

		// now we need to write the scalers.
		if (isLargeScalers())
		{
			stream.write(scalers_.ptr(),scalers_.size());
		}
		else
		{
			for (auto s : scalers_)
			{
				Vec3<uint8_t> s8;

				s8.x = safe_static_cast<uint8_t, uint16_t>(s.x);
				s8.y = safe_static_cast<uint8_t, uint16_t>(s.y);
				s8.z = safe_static_cast<uint8_t, uint16_t>(s.z);

				stream.write(s8);
			}
		}

		stream.write(relativeMin);
		stream.write(range_);
	}
}

void AnimCompiler::Position::appendFullPos(const Vec3f& pos)
{
	fullPos_.append(pos);
}

void AnimCompiler::Position::setBasePosition(const Vec3f& basePos)
{
	basePosition_ = basePos;
}

size_t AnimCompiler::Position::numPosFrames(void) const
{
	return posDeltas_.size();
}

bool AnimCompiler::Position::hasData(void) const
{
	return posDeltas_.isNotEmpty();
}

bool AnimCompiler::Position::isFullFrames(void) const
{
	return posDeltas_.size() == fullPos_.size();
}

bool AnimCompiler::Position::isLargeScalers(void) const
{
	return largeScalers_;
}

Vec3f AnimCompiler::Position::minRelative(void) const
{
	return min_ - basePosition_;
}

const Vec3f& AnimCompiler::Position::min(void) const
{
	return min_;
}

const Vec3f& AnimCompiler::Position::range(void) const
{
	return range_;
}


const Vec3f& AnimCompiler::Position::basePos(void) const
{
	return basePosition_;
}


void AnimCompiler::Position::calculateDeltas(const float posError)
{
	posDeltas_.clear();
	posDeltas_.reserve(fullPos_.size());

	min_ = Vec3f::max();
	max_ = Vec3f::min();

	/*
		This logic looks for linera movement and only stores info when the
		the translation deviates more than 'posError'.

		For example if the bone moves in the same direction at the same speed every frame
		we don't need to bother storing.

		if the rate of movement keeps chaning frames will need to be stored.
	*/
	
	// we don't do anthing until we have moved.
	const int32_t totalFrames = static_cast<int32_t>(fullPos_.size());
	int32_t frame = 0;

	auto addFrame = [&](int32_t frameIdx) {

		const Vec3f& pos = fullPos_[frameIdx];

		min_.checkMin(pos);
		max_.checkMax(pos);

		PosDelta& deltaEntry = posDeltas_.AddOne();
		deltaEntry.worldPos = pos;
		deltaEntry.frame = frameIdx;
	};

	// find the first frame we move
	for (frame = 0; frame < totalFrames; frame++)
	{
		auto delta = fullPos_[frame] - basePosition_;

		if (!delta.compare(Vec3f::zero(), posError))
		{
			// store the frame before as that's when movement started
			// if it's the first frame tho, we are 'jumping' the bone to a start location.
			if (frame > 0) 
			{
				--frame;
			}

			addFrame(frame);
			break;
		}
	}

	if (frame < totalFrames)
	{
		// we stored movement data for current frame
		// now we keep moving forward checking pos error against last frame.
		int32_t lastStoredFrame = frame;
		
		for (++frame; frame < totalFrames; ++frame)
		{
			const Vec3f& lastStoredPos = fullPos_[lastStoredFrame];
			const Vec3f& singleDelta = fullPos_[lastStoredFrame + 1] - lastStoredPos;
			const Vec3f& curFramePos = fullPos_[frame];

			int32_t numFrames = frame - lastStoredFrame;
			
			// if continue at same rate we would have this delta
			Vec3f linDelta = singleDelta * numFrames;
			// the actual delta.
			Vec3f delta = curFramePos - lastStoredPos;

			// have we dirifted?
			if (!delta.compare(linDelta, posError))
			{
				// lets store the frame before we drifted,
				lastStoredFrame = frame - 1;

				addFrame(lastStoredFrame);
			}
		}

		// do we always need a end frame?
		if (lastStoredFrame != totalFrames - 1)
		{
			addFrame(totalFrames - 1);
		}
	}

	if (posDeltas_.isEmpty())
	{
		min_ = Vec3f::zero();
		max_ = Vec3f::zero();
		return;
	}
	else
	{
		X_ASSERT(min_ != Vec3f::max(), "Infinate range")();
		X_ASSERT(max_ != Vec3f::min(), "Infinate range")();
	}

	buildScalers(posError);
}


void AnimCompiler::Position::buildScalers(const float posError)
{
	range_ = max_ - min_;

	uint32_t segments = (1 << 8) - 1;

	// work out if we can use 8bit scalers with selected posError.
	Vec3f rangePercision = range_ / std::numeric_limits<uint8_t>::max();
	if (rangePercision.x > posError || rangePercision.y > posError || rangePercision.z > posError) {
		segments = (1 << 16) - 1; // need 16 bit.

		largeScalers_ = true;
	}
	else {
		largeScalers_ = false;
	}

	Vec3f segmentsize(range_ / Vec3f(static_cast<float>(segments)));

	scalers_.clear();
	scalers_.reserve(posDeltas_.size());

	for (auto& deltaEntry : posDeltas_)
	{
		// we know want to know the scaler needed to get the world pos.
		Vec3f worldDelta = deltaEntry.worldPos - min_;
		Vec3f scalerF = (worldDelta / segmentsize);
	

		Scaler scaler;
		scaler.x = static_cast<Scaler::value_type>(math<float>::round(scalerF.x));
		scaler.y = static_cast<Scaler::value_type>(math<float>::round(scalerF.y));
		scaler.z = static_cast<Scaler::value_type>(math<float>::round(scalerF.z));

		scalers_.append(scaler);
	}

#if X_DEBUG
	if (!largeScalers_)
	{
		// i know what the max is, i just think numeric_limits makes code intention more clear
		auto max = std::numeric_limits<uint8_t>::max(); 
		for (auto& scaler : scalers_)
		{
			X_ASSERT(scaler.x <= max && scaler.x <= max && scaler.x <= max, "Invalid scaler")(scaler.x, scaler.y, scaler.z);
		}
	}
#endif
}


// ====================================================


AnimCompiler::Angle::Angle(core::MemoryArenaBase* arena) :
	fullAngles_(arena),
	angles_(arena)
{
	fullAngles_.setGranularity(128);
}

void AnimCompiler::Angle::save(core::ByteStream& stream) const
{
	int16_t numAngle = safe_static_cast<uint16_t>(angles_.size());

	stream.write(numAngle);

	if (numAngle > 0)
	{
		if (!isFullFrames() && angles_.size() > 1)
		{
			size_t numFrames = fullAngles_.size();

			// frame numbers are 8bit if total anim frames less than 255
			if (numFrames <= std::numeric_limits<uint8_t>::max())
			{
				for (const auto& a : angles_)
				{
					uint8_t frame = safe_static_cast<uint8_t, uint32_t>(a.frame);
					stream.write(frame);
				}
			}
			else
			{
				for (const auto& a : angles_)
				{
					uint16_t frame = safe_static_cast<uint16_t, uint32_t>(a.frame);
					stream.write(frame);
				}
			}
		}

		// write angles
		for (const auto& a : angles_)
		{
			// compressed quat.
			XQuatCompressedf quatf(a.angle);
			stream.write(quatf);
		}
	}
}

void AnimCompiler::Angle::appendFullAng(const Quatf& ang)
{
	fullAngles_.append(ang);
}

void AnimCompiler::Angle::setBaseOrient(const Quatf& ang)
{
	baseOrient_ = ang;
}

size_t AnimCompiler::Angle::numAngleFrames(void) const
{
	return angles_.size();
}


bool AnimCompiler::Angle::hasData(void) const
{
	return angles_.isNotEmpty();
}

bool AnimCompiler::Angle::isFullFrames(void) const
{
	// we have a angle for every frame ?
#if 1
	return false;
#else
	return fullAngles_.size() == angles_.size();
#endif
}

void AnimCompiler::Angle::calculateDeltas(const float angError)
{
	angles_.clear();
	angles_.reserve(fullAngles_.size());

	Quatf curAng = baseOrient_;
	int32_t frame = 0;

	for (auto& ang : fullAngles_)
	{
		Quatf delta = ang.diff(curAng);

		float pitchDelta = delta.getPitch();
		float rollDelta = delta.getRoll();
		float yawDelta = delta.getYaw();

		bool pitchPass = math<float>::abs(pitchDelta) > angError;
		bool rollPass = math<float>::abs(rollDelta) > angError;
		bool yawPass = math<float>::abs(yawDelta) > angError;

		if (pitchPass || rollPass || yawPass)
		{
			axisChanges_[0] |= pitchPass;
			axisChanges_[1] |= rollPass;
			axisChanges_[2] |= yawPass;

			// add it :)
			AngleFrame a;
			a.frame = frame;
			a.angle = ang;
			angles_.append(a);

			curAng = ang;
		}

		frame++;
	}
}

// ====================================================

AnimCompiler::Bone::Bone(core::MemoryArenaBase* arena) :
	pos(arena),
	ang(arena)
{

}


bool AnimCompiler::Bone::hasData(void) const
{
	return ang.hasData() || pos.hasData();
}


// ----------------------------------------------------

AnimCompiler::AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton) :
	arena_(arena),
	inter_(inter),
	skelton_(skelton),
	scale_(1.f),
	type_(AnimType::RELATIVE),
	bones_(arena),
	stats_(arena)
{

}


AnimCompiler::~AnimCompiler()
{

}

void AnimCompiler::printStats(bool verbose) const
{
	stats_.print();

	if (verbose)
	{
		X_LOG0("Anim", "Per bone info:");

		for (auto& bone : bones_)
		{
			auto min = bone.pos.minRelative();
			auto& r = bone.pos.range();

			core::StackString256 info;

			auto& basePos = bone.pos.basePos();

			X_LOG0("Anim", "-> \"%s\" basePos(^6%g^7,^6%g^7,^6%g^7)", bone.name.c_str(), basePos.x, basePos.y, basePos.z);
			X_LOG_BULLET;

			X_LOG0("Anim", "ang: ^6%2" PRIuS "^7 full: ^6%d",
				bone.ang.numAngleFrames(), bone.ang.isFullFrames());

			X_LOG0("Anim", "pos: ^6%2" PRIuS "^7 full: ^6%d^7 largeS: ^6%d ^7min(%g,%g,%G) range(%g,%g,%g)",
				bone.pos.numPosFrames(), bone.pos.isFullFrames(), bone.pos.isLargeScalers(),
				min.x, min.y, min.z, r.x, r.y, r.z);
		}
	}
}

void AnimCompiler::setScale(float scale)
{
	scale_ = scale;
}

void AnimCompiler::setLooping(bool loop)
{
	if (loop) {
		flags_.Set(CompileFlag::LOOPING);
	}
	else {
		flags_.Remove(CompileFlag::LOOPING);
	}
}

void AnimCompiler::setAnimType(AnimType::Enum type)
{
	type_ = type;
}

bool AnimCompiler::compile(const core::Path<char>& filePath, const float posError, const float angError)
{
	return compile(core::Path<wchar_t>(filePath), posError, angError);
}

bool AnimCompiler::compile(const core::Path<wchar_t>& path, const float posError, const float angError)
{
	if (type_ != AnimType::RELATIVE) {
		X_ERROR("Anim", "Compiling of none relative animations is not yet supported");
		return false;
	}

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
		X_ERROR("Anim", "inter anim fps exceeds max: %" PRIu32, anim::ANIM_MAX_FPS);
		return false;
	}
	if (inter_.getNumFrames() > anim::ANIM_MAX_FRAMES) {
		X_ERROR("Anim", "inter anim exceeds max frames: %" PRIu32, anim::ANIM_MAX_FRAMES);
		return false;
	}

	core::StopWatch timer;

	loadInterBones();
	dropMissingBones();

	if (bones_.isEmpty()) {
		X_WARNING("Anim", "skipping compile of anim, inter anim and model skelton have no bones in common");
		return true;
	}

	// load base bone positions from model skelton.
	loadBaseData();
	processBones(posError, angError);

	// cull any without data.
	dropNullBones();

	// build some stats.
	stats_.scale = scale_;
	stats_.numFrames = inter_.getNumFrames();
	stats_.fps = inter_.getFps();
	stats_.totalBones = bones_.size();
	stats_.compileTime = timer.GetTimeVal();

	for (const auto& bone : bones_)
	{
		stats_.totalBonesAngleData += static_cast<int32_t>(bone.ang.hasData());
		stats_.totalBonesPosData += static_cast<int32_t>(bone.pos.hasData());
	}

	return save(path);
}

bool AnimCompiler::save(const core::Path<wchar_t>& path)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	core::Path<wchar_t> fullPath(path);
	fullPath.setExtension(anim::ANIM_FILE_EXTENSION_W);

	core::fileModeFlags mode;
	mode.Set(core::fileMode::RECREATE);
	mode.Set(core::fileMode::WRITE);

	core::XFileScoped file;
	if (!file.openFile(fullPath.c_str(), mode)) {
		X_ERROR("Anim", "Failed to open output file for compiled animation: \"%s\"",
			fullPath.c_str());
		return false;
	}


	anim::AnimHeader hdr;
	hdr.version = anim::ANIM_VERSION;
	hdr.type = type_;
	hdr.numBones = safe_static_cast<uint8_t, size_t>(bones_.size());
	hdr.numFrames = safe_static_cast<uint16_t, uint32_t>(inter_.getNumFrames());
	hdr.fps = safe_static_cast<uint16_t, uint32_t>(inter_.getFps());

	if (flags_.IsSet(CompileFlag::LOOPING)) {
		hdr.flags.Set(AnimFlag::LOOP);
	}

	core::ByteStream stream(arena_);

	// write the bone names.
	for (const auto& bone : bones_)
	{
		stream.write(bone.name.c_str(), core::strUtil::StringBytesIncNull(bone.name));
	}

	// now we save the data.
	for (const auto& bone : bones_)
	{
		bone.ang.save(stream);
		bone.pos.save(stream);
	}

	hdr.dataSize = safe_static_cast<uint32_t>(stream.size());
	
	if (file.writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("Anim", "Failed to write header");
		return false;
	}

	if (file.write(stream.data(), stream.size()) != stream.size()) {
		X_ERROR("Anim", "Failed to write data");
		return false;
	}

	return true;
}


void AnimCompiler::loadInterBones(void)
{
	// make a copy of all the bones names in anim file.
	bones_.resize(inter_.getNumBones(), Bone(arena_));

	for (size_t i = 0; i < inter_.getNumBones(); i++)
	{
		const anim::Bone& interBone = inter_.getBone(i);
		const size_t dataNum = interBone.data.size();

		Bone& bone = bones_[i];
		bone.name = interBone.name;

		// load pos / angles.
		for (size_t x = 0; x < dataNum; x++)
		{
			bone.pos.appendFullPos(interBone.data[x].position * scale_);
			bone.ang.appendFullAng(interBone.data[x].rotation);
		}
	}
}

void AnimCompiler::dropMissingBones(void)
{
	// drop any bones that are not in the model file.
	for (size_t i = 0; i < bones_.size(); i++)
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
			stats_.droppedBoneNames.append(name);
			bones_.removeIndex(i);
		}
	}
}

void AnimCompiler::dropNullBones(void)
{
	// drop any bones that have no data.
	for (size_t i = 0; i < bones_.size(); i++)
	{
		auto& bone = bones_[i];

		if(!bone.hasData())
		{
			// remove it.
			stats_.droppedBoneNames.append(bone.name);
			bones_.removeIndex(i);
		}
	}
}

void AnimCompiler::loadBaseData(void)
{
	for (size_t i = 0; i < bones_.size(); i++)
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
				bone.pos.setBasePosition(pos);
				bone.ang.setBaseOrient(angle.asQuat());
				break;
			}
		}

		if (x == numModelBones)
		{
			X_ASSERT_UNREACHABLE();
		}
	}
}


void AnimCompiler::processBones(const float posError, const float angError)
{
	for (auto& bone : bones_)
	{
		bone.pos.calculateDeltas(posError);
		bone.ang.calculateDeltas(angError);
	}
}

X_NAMESPACE_END