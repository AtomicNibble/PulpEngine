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

	bool LoadCompiledSkelton(const core::Path<char>& path);
	bool LoadCompiledSkelton(const core::Path<wchar_t>& path);

	size_t getNumBones(void) const;

	const char* getBoneName(size_t idx) const;

	const Quatf getBoneAngle(size_t idx) const;
	const Vec3f getBonePos(size_t idx) const;

private:
	typedef core::Array<core::StackString<model::MODEL_MAX_BONE_NAME_LENGTH>> TagNames;
	typedef core::Array<uint16_t> TagNameIdx;
	typedef core::Array<uint8_t> TagTree;
	typedef core::Array<Quatf> TagAngles;
	typedef core::Array<Vec3f> TagPos;

	core::MemoryArenaBase* arena_;

	size_t numBones_;

	// built.
	TagNames tagNames_;
	// from file.
	TagNameIdx nameIdx_;
	TagTree tree_;
	TagAngles angles_;
	TagPos positions_;
};


X_NAMESPACE_END