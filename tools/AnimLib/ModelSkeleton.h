#pragma once


#include <Containers\Array.h>
#include <Math\XQuatCompressed.h>
#include <String\Path.h>

#include <IModel.h>

X_NAMESPACE_BEGIN(model)


class ModelSkeleton
{
public:
	ModelSkeleton(core::MemoryArenaBase* arena);
	~ModelSkeleton();

	bool LoadSkelton(core::Path<char>& path);

private:
	typedef core::Array<core::StackString<model::MODEL_MAX_BONE_NAME_LENGTH>> TagNames;
	typedef core::Array<uint16_t> TagNameIdx;
	typedef core::Array<uint8_t> TagTree;
	typedef core::Array<XQuatCompressedf> TagAngles;
	typedef core::Array<Vec3f> TagPos;

	// built.
	TagNames tagNames_;
	// from file.
	TagNameIdx nameIdx_;
	TagTree tree_;
	TagAngles angles_;
	TagPos positions_;
};


X_NAMESPACE_END