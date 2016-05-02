#include "stdafx.h"
#include "ModelSkeleton.h"

#include <IModel.h>

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(model)


bool ModelHeader::isValid(void) const
{
	if (version != MODEL_VERSION) {
		X_ERROR("Model", "model version is invalid. FileVer: %i RequiredVer: %i",
			version, MODEL_VERSION);
	}

	return version == MODEL_VERSION &&
		(numBones + numBlankBones) > 0 &&
		numLod > 0 &&
		numLod <= MODEL_MAX_LODS &&
		materialNameDataSize > 0 &&
		subDataSize > 0;
}

// ==================================


ModelSkeleton::ModelSkeleton(core::MemoryArenaBase* arena) :
	tagNames_(arena),

	nameIdx_(arena),
	tree_(arena),
	angles_(arena),
	positions_(arena)
{
	numBones_ = 0;
}

ModelSkeleton::~ModelSkeleton()
{


}

bool ModelSkeleton::LoadSkelton(const core::Path<char>& filePath)
{
	return LoadSkelton(core::Path<wchar_t>(filePath));
}

bool ModelSkeleton::LoadSkelton(const core::Path<wchar_t>& filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}

	core::Path<wchar_t> path(filePath);
	path.setExtension(model::MODEL_FILE_EXTENSION_W);

	core::fileModeFlags mode;
	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::RANDOM_ACCESS);


	core::XFileScoped file;
	if (!file.openFile(path.c_str(), mode)) {
		X_ERROR("Model", "Failed to open model for skelton load");
		return false;
	}

	ModelHeader hdr;

	// read header;
	if(file.readObj(hdr) != sizeof(hdr)) {
		X_ERROR("Model", "failed to read model header");
		return false;
	}

	if (!hdr.isValid()) {
		X_ERROR("Model", "model header is invalid");
		return false;
	}

	if (!hdr.flags.IsSet(ModelFlags::LOOSE)) {
		X_ERROR("Model", "can't load model skelton from none loose model");
		return false;
	}

	// we only care about none black bones.
	if (hdr.numBones > 0)
	{
		// read bone data.
		nameIdx_.resize(hdr.numBones);
		tree_.resize(hdr.numBones);
		angles_.resize(hdr.numBones);
		positions_.resize(hdr.numBones);

		numBones_ = hdr.numBones;

		// the tag names are after the material names.
		const long tagNameOffset = sizeof(hdr) + hdr.materialNameDataSize;
		// start of name index and tree
		const long tagDataOffset = sizeof(hdr) + (hdr.dataSize - hdr.subDataSize);

		char TagNameBuf[model::MODEL_MAX_BONE_NAME_LENGTH * model::MODEL_MAX_BONES];

		file.seek(tagNameOffset, core::SeekMode::SET);
		if (!file.read(TagNameBuf, hdr.tagNameDataSize) != hdr.tagNameDataSize) {
			X_ERROR("Model", "failed to read tag buf");
			return false;
		}

		core::MemCursor<char> tag_name_cursor(TagNameBuf, hdr.tagNameDataSize);
		core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

		for (size_t i = 0; i < hdr.numBones; i++)
		{
			while (!tag_name_cursor.isEof() && tag_name_cursor.get<char>() != '\0') {
				name.append(tag_name_cursor.getSeek<char>(), 1);
			}

			if (!tag_name_cursor.isEof()) {
				++tag_name_cursor;
			}

			tagNames_.append(name);

			name.clear();
		}

		file.seek(tagDataOffset, core::SeekMode::SET);

		if (!file.readObjs(nameIdx_.ptr(), nameIdx_.size())) {
			X_ERROR("Model", "failed to read name indexes");
			return false;
		}
		if (!file.readObjs(tree_.ptr(), tree_.size())) {
			X_ERROR("Model", "failed to read tree");
			return false;
		}
		if (!file.readObjs(angles_.ptr(), angles_.size())) {
			X_ERROR("Model", "failed to read angles");
			return false;
		}
		if (!file.readObjs(positions_.ptr(), positions_.size())) {
			X_ERROR("Model", "failed to read pos data");
			return false;
		}
	}

	return true;
}


size_t ModelSkeleton::getNumBones(void) const
{
	return numBones_;
}

const char* ModelSkeleton::getBoneName(size_t idx) const
{
	return tagNames_[idx].c_str();
}

const XQuatCompressedf ModelSkeleton::getBoneAngle(size_t idx) const
{
	return angles_[idx];
}

const Vec3f ModelSkeleton::getBonePos(size_t idx) const
{
	return positions_[idx];
}

X_NAMESPACE_END