#include "stdafx.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)

const float AnimCompiler::DEFAULT_POS_ERRR = 0.05f;
const float AnimCompiler::DEFAULT_ANGLE_ERRR = 0.005f;

AnimCompiler::Position::Position(core::MemoryArenaBase* arena) :
	fullPos_(arena),
	posDeltas_(arena),
	scalers_(arena)
{
	fullPos_.setGranularity(128);

	largeScalers_ = false;
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

bool AnimCompiler::Position::isLargeScalers(void) const
{
	return largeScalers_;
}

bool AnimCompiler::Position::isFullFrames(void) const
{
	return posDeltas_.size() == fullPos_.size();
}

const Vec3f& AnimCompiler::Position::min(void) const
{
	return min_;
}

const Vec3f& AnimCompiler::Position::range(void) const
{
	return range_;
}

void AnimCompiler::Position::save(core::XFile* pFile) const
{
	X_ASSERT_NOT_NULL(pFile);

	int16_t numPos = safe_static_cast<uint16_t, size_t>(posDeltas_.size());

	pFile->writeObj(numPos);

	if (numPos == 0)
	{
		pFile->writeObj(min_); // just write min pos.
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
					uint8_t frame = safe_static_cast<uint8_t,uint32_t>(delta.frame);
					pFile->writeObj(frame);
				}
			}
			else
			{
				for (const PosDelta& delta : posDeltas_)
				{
					uint16_t frame = safe_static_cast<uint16_t, uint32_t>(delta.frame);
					pFile->writeObj(frame);
				}
			}
		}

		// now we need to write the scalers.
		if (isLargeScalers())
		{
			pFile->write(scalers_.ptr(),
				safe_static_cast<uint32_t, size_t>(scalers_.size() * sizeof(Scaler)));
		}
		else
		{
			for (auto s : scalers_)
			{
				Vec3<uint8_t> s8;

				s8.x = safe_static_cast<uint8_t, uint16_t>(s.x);
				s8.y = safe_static_cast<uint8_t, uint16_t>(s.y);
				s8.z = safe_static_cast<uint8_t, uint16_t>(s.z);

				pFile->writeObj(s8);
			}
		}

		pFile->writeObj(min_);
		pFile->writeObj(range_);
	}
}

void AnimCompiler::Position::CalculateDeltas(const float posError)
{
	posDeltas_.clear();
	posDeltas_.reserve(fullPos_.size());

	// we only store a delta if it's diffrent.
	int32_t frame = 0;
	Vec3f curPos = basePosition_;

	min_ = Vec3f::max();
	max_ = Vec3f::min();

	for (auto& pos : fullPos_)
	{
		Vec3f delta = curPos - pos;

		if (!delta.compare(Vec3f::zero(), posError))
		{
			// only store min max for positions we are going to move to.
			min_.checkMin(pos);
			max_.checkMax(pos);

			// we moved enougth.
			PosDelta& deltaEntry = posDeltas_.AddOne();
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
}


// ====================================================


AnimCompiler::Angle::Angle(core::MemoryArenaBase* arena) :
	fullAngles_(arena),
	angles_(arena)
{
	fullAngles_.setGranularity(128);
}


void AnimCompiler::Angle::appendFullAng(const Quatf& ang)
{
	fullAngles_.append(ang);
}

void AnimCompiler::Angle::setBaseOrient(const Quatf& ang)
{
	baseOrient_ = ang;
}

bool AnimCompiler::Angle::isFullFrames(void) const
{
	return fullAngles_.size() == angles_.size();
}

void AnimCompiler::Angle::save(core::XFile* pFile) const
{
	X_ASSERT_NOT_NULL(pFile);

	int16_t numAngle = safe_static_cast<uint16_t, size_t>(angles_.size());

	pFile->writeObj(numAngle);

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
					pFile->writeObj(frame);
				}
			}
			else
			{
				for (const auto& a : angles_)
				{
					uint16_t frame = safe_static_cast<uint16_t, uint32_t>(a.frame);
					pFile->writeObj(frame);
				}
			}
		}

		// write angles
		for (const auto& a : angles_)
		{
			// compressed quat.
			XQuatCompressedf quatf(a.angle);
			pFile->writeObj(quatf);
		}
	}
}

void AnimCompiler::Angle::CalculateDeltas(const float angError)
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

AnimCompiler::AnimCompiler(core::MemoryArenaBase* arena, const InterAnim& inter, const model::ModelSkeleton& skelton) :
	arena_(arena),
	inter_(inter),
	skelton_(skelton),
	type_(AnimType::RELATIVE),
	bones_(arena)
{

}


AnimCompiler::~AnimCompiler()
{

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
	processBones(posError, angError);

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

	if (flags_.IsSet(CompileFlag::LOOPING)) {
		hdr.flags.Set(AnimFlag::LOOP);
	}

	hdr.type = type_;
	hdr.numBones = safe_static_cast<uint8_t, size_t>(bones_.size());
	hdr.numFrames = safe_static_cast<uint16_t, uint32_t>(inter_.getNumFrames());
	hdr.fps = safe_static_cast<uint16_t, uint32_t>(inter_.getFps());

	file.writeObj(hdr);

	// write the bone names.
	for (const auto& bone : bones_)
	{
		file.writeString(bone.name);
	}

	// now we save the data.
	for (const auto& bone : bones_)
	{
		bone.ang.save(file.GetFile());
		bone.pos.save(file.GetFile());
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
			bone.pos.appendFullPos(interBone.data[x].position);
			bone.ang.appendFullAng(interBone.data[x].rotation);
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
		bone.pos.CalculateDeltas(posError);
		bone.ang.CalculateDeltas(angError);
	}
}

X_NAMESPACE_END