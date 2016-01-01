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

}

ModelSkeleton::~ModelSkeleton()
{


}


bool ModelSkeleton::LoadSkelton(core::Path<char>& filePath)
{
	if (filePath.isEmpty()) {
		return false;
	}

	filePath.setExtension(model::MODEL_FILE_EXTENSION);

	FILE* f = nullptr;

	errno_t err = fopen_s(&f, filePath.c_str(), "rb");
	if (!f) {
		return false;
	}


	ModelHeader hdr;

	// read header;
	if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
		X_ERROR("Model", "failed to read model header");
		return false;
	}

	if (!hdr.isValid()) {
		X_ERROR("Model", "model header is invalid");
		::fclose(f);
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


		// the tag names are after the material names.
		const long tagNameOffset = sizeof(hdr) + hdr.materialNameDataSize;
		// start of name index and tree
		const long tagDataOffset = sizeof(hdr) + (hdr.dataSize - hdr.subDataSize);

		char TagNameBuf[model::MODEL_MAX_BONE_NAME_LENGTH * model::MODEL_MAX_BONES];

		::fseek(f, tagNameOffset, SEEK_SET);
		::fread(TagNameBuf, 1, hdr.tagNameDataSize, f);

		core::MemCursor<char> tag_name_cursor(TagNameBuf, hdr.tagNameDataSize);
		core::StackString<MODEL_MAX_BONE_NAME_LENGTH> name;

		for (size_t i = 0; i < hdr.numBones; i++)
		{
			while (!tag_name_cursor.isEof() && tag_name_cursor.get<char>() != '\0') {
				name.append(tag_name_cursor.getSeek<char>(), 1);
			}

			if (!tag_name_cursor.isEof())
				++tag_name_cursor;

			tagNames_.append(name);

			name.clear();
		}

		::fseek(f, tagDataOffset, SEEK_SET);
		::fread(nameIdx_.ptr(), sizeof(TagNameIdx::Type), nameIdx_.size(), f);
		::fread(tree_.ptr(), sizeof(TagTree::Type), tree_.size(), f);
		::fread(angles_.ptr(), sizeof(TagAngles::Type), angles_.size(), f);
		::fread(positions_.ptr(), sizeof(TagPos::Type), positions_.size(), f);


	}

	::fclose(f);
	return true;
}


X_NAMESPACE_END