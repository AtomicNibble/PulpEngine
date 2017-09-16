#include "stdafx.h"
#include "anim_inter.h"

#include <IFileSys.h>
#include <IModel.h>
#include <String\Lexer.h>

X_NAMESPACE_BEGIN(anim)


Bone::Bone(core::MemoryArenaBase* arena) :
	data(arena)
{

}

Bone::~Bone()
{

}

// =================================

InterAnim::InterAnim(core::MemoryArenaBase* arena) : 
	arena_(arena),
	bones_(arena)
{

}


bool InterAnim::load(core::Path<char>& file)
{
	return load(core::Path<wchar_t>(file));
}


bool InterAnim::load(core::Path<wchar_t>& filePath)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	// swap a woggle watch it toggle!
	if (filePath.isEmpty()) {
		X_ERROR("InterAnim", "invalid path");
		return false;
	}

	filePath.setExtension(anim::ANIM_INTER_FILE_EXTENSION_W);

	core::fileModeFlags mode;
	mode.Set(core::fileMode::READ);

	core::XFileScoped file;
	if (file.openFile(filePath.c_str(), mode)) {
		X_ERROR("InterAnim", "failed to open file: %ls", filePath.c_str());
		return false;
	}

	const size_t fileSize = safe_static_cast<size_t, uint64_t>(file.remainingBytes());
	if (fileSize < 1) {
		X_ERROR("InterAnim", "file size invalid");
		return false;
	}

	core::Array<char> fileData(arena_);
	fileData.resize(fileSize);

	const size_t bytesRead = file.read(fileData.ptr(), fileSize);
	if (bytesRead != fileSize) {
		X_ERROR("InterAnim", "failed to read file data. got: %" PRIuS " requested: %" PRIuS, bytesRead, fileSize);
		return false;
	}

	core::XLexer lex(fileData.begin(), fileData.end());

	return ParseData(lex);
}

bool InterAnim::load(const core::Array<uint8_t>& fileData)
{
	core::XLexer lex(reinterpret_cast<const char*>(fileData.begin()), reinterpret_cast<const char*>(fileData.end()));

	return ParseData(lex);
}

bool InterAnim::load(const core::ByteStream& fileData)
{
	core::XLexer lex(reinterpret_cast<const char*>(fileData.begin()), reinterpret_cast<const char*>(fileData.end()));

	return ParseData(lex);
}


int32_t InterAnim::getNumFrames(void) const
{
	return numFrames_;
}

int32_t InterAnim::getFps(void) const
{
	return fps_;
}

size_t InterAnim::getNumBones(void) const
{
	return bones_.size();
}

const Bone& InterAnim::getBone(size_t idx) const
{
	return bones_[idx];
}



bool InterAnim::ParseData(core::XLexer& lex)
{
	int32_t version, numBones;

	/*
	Example header:
		VERSION 1
		BONES 46
		FRAMES 99
		FPS 30
	*/

	if (!ReadheaderToken(lex, "VERSION", version)) {
		return false;
	}
	if (!ReadheaderToken(lex, "BONES", numBones)) {
		return false;
	}
	if (!ReadheaderToken(lex, "FRAMES", numFrames_)) {
		return false;
	}
	if (!ReadheaderToken(lex, "FPS", fps_)) {
		return false;
	}

	// check dat version number slut.
	if (version < anim::ANIM_INTER_VERSION) {
		X_ERROR("InterAnim", "InterAnim file version is too old: %" PRIi32 " required: %" PRIu32,
			version, anim::ANIM_INTER_VERSION);
		return false;
	}
	// limit checks
	if (fps_ > anim::ANIM_MAX_FPS) {
		X_ERROR("InterAnim", "InterAnim file fps is too high: %" PRIi32 " max: %" PRIu32,
			fps_, anim::ANIM_MAX_FPS);
		return false;
	}
	if (fps_ < anim::ANIM_MIN_FPS) {
		X_ERROR("InterAnim", "InterAnim file fps is too low: %" PRIi32 " min: %" PRIu32,
			fps_, anim::ANIM_MIN_FPS);
		return false;
	}
	if (numBones > anim::ANIM_MAX_BONES) {
		X_ERROR("InterAnim", "InterAnim file has too many bones: %" PRIi32 " max: %" PRIu32,
			numBones, anim::ANIM_MAX_BONES);
		return false;
	}

	// how many bones!
	if (numBones < 1) {
		X_ERROR("InterAnim", "animation has zero bones");
		return false;
	}
	// we need some frames to play some games.
	if (numFrames_ < 1) {
		X_ERROR("InterAnim", "animation has zero frames");
		return false;
	}
	// fluffy pig sausages
	if (fps_ < 1) {
		X_ERROR("InterAnim", "animation has fps lower than 1");
		return false;
	}

	// fill my cofin.
	if (!ReadBones(lex, numBones)) {
		X_ERROR("InterAnim", "failed to parse bone data");
		return false;
	}

	// data time.
	if (!ReadFrameData(lex, numBones)) {
		X_ERROR("InterAnim", "failed to parse frame data");
		return false;
	}

	if (!lex.isEOF(true)) {
		X_ERROR("InterAnim", "trailing data in file");
		return false;
	}

	return true;
}


bool InterAnim::ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut)
{
	core::XLexToken token(nullptr, nullptr);

	valOut = 0;

	if (!lex.SkipUntilString(pName)) {
		X_ERROR("InterAnim", "Failed to find '%s' token", pName);
		return false;
	}

	// get value
	if (!lex.ReadToken(token)) {
		X_ERROR("InterAnim", "Failed to read '%s' value", pName);
		return false;
	}

	if (token.GetType() != core::TokenType::NUMBER) {
		X_ERROR("InterAnim", "Failed to read '%s' value, it's not of interger type", pName);
		return false;
	}

	valOut = token.GetIntValue();
	return true;
}


bool InterAnim::ReadBones(core::XLexer& lex, int32_t numBones)
{
	bones_.reserve(numBones);

	core::XLexToken token(nullptr, nullptr);

	// BONE "name"
	for (int32_t i = 0; i < numBones; i++)
	{
		if (!lex.ReadToken(token)) {
			X_ERROR("InterAnim", "Failed to read 'BONE' token");
			return false;
		}

		if (!token.isEqual("BONE")) {
			X_ERROR("InterAnim", "Failed to read 'BONE' token");
			return false;
		}

		// read the string
		if (!lex.ReadTokenOnLine(token)) {
			X_ERROR("InterAnim", "Failed to read 'BONE' token");
			return false;
		}

		Bone bone(arena_);
		bone.name = core::string(token.begin(), token.end());

		// validate the names
		if (bone.name.length() < 1) {
			X_ERROR("InterAnim", "bone name too short: \"%s\"", bone.name.c_str());
			return false;
		}
		if (bone.name.length() > model::MODEL_MAX_BONE_NAME_LENGTH) {
			X_ERROR("InterAnim", "bone name too long: \"%s\" max: %" PRIu32,
				bone.name.c_str(), model::MODEL_MAX_BONE_NAME_LENGTH);
			return false;
		}

#if X_MODEL_BONES_LOWER_CASE_NAMES
		bone.name.toLower();
#endif // !X_MODEL_BONES_LOWER_CASE_NAMES

		bones_.append(bone);
	}

	return true;
}


bool InterAnim::ReadFrameData(core::XLexer& lex, int32_t numBones)
{
	X_ASSERT(numBones == bones_.size(), "bones size should alread equal numbones")(numBones, bones_.size());

	// for each bone there is numFrames worth of data.
	// in bone order.
	for (auto& bone : bones_)
	{
		// data has a start tag.
		if (!lex.SkipUntilString("BONE_DATA")) {
			X_ERROR("InterAnim", "missing BONE_DATA tag");
			return false;
		}

		/*
		Example:
		POS 0 0 0
		SCALE 1 1 1
		ANG 0 -0.21869613 0 0.97579301
		*/

		Bone::BoneData& data = bone.data;

		data.reserve(numFrames_);
		data.clear();

		core::XLexToken token(nullptr, nullptr);

		for (int32_t i = 0; i < numFrames_; i++)
		{
			FrameData& fd = data.AddOne();

			// pos
			if (!lex.ExpectTokenString("POS")) {
				return false;
			}
			if (!lex.Parse1DMatrix(3, &fd.position[0])) {
				return false;
			}

			// scale
			if (!lex.ExpectTokenString("SCALE")) {
				return false;
			}
			if (!lex.Parse1DMatrix(3, &fd.scale[0])) {
				return false;
			}

			// angles
			if (!lex.ExpectTokenString("ANG")) {
				return false;
			}
			if (!lex.Parse2DMatrix(3,3, &fd.rotation[0])) {
				return false;
			}

		}
	}

	return true;
}

X_NAMESPACE_END