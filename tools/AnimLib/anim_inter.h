#pragma once


#include <Containers\Array.h>
#include <String\Path.h>

#include <IAnimation.h>

X_NAMESPACE_DECLARE(core,
class XLexer
);

X_NAMESPACE_BEGIN(anim)


struct FrameData
{
	Vec3f position;
	Vec3f scale;
	Quatf rotation;
};


struct Bone
{
	typedef core::Array<FrameData> BoneData;
public:
	Bone(core::MemoryArenaBase* arena);
	~Bone();

public:
	core::string name; // using this requires gEnv->pArena
	BoneData data;
};

class InterAnim
{
public:
	InterAnim(core::MemoryArenaBase* arena);
	~InterAnim() = default;

	bool LoadFile(core::Path<wchar_t>& file);

	int32_t getNumFrames(void) const;
	int32_t getFps(void) const;
	size_t getNumBones(void) const;
	const Bone& getBone(size_t idx) const;

private:
	bool ParseData(core::XLexer& lex);
	bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut);
	bool ReadBones(core::XLexer& lex, int32_t numBones);
	bool ReadFrameData(core::XLexer& lex, int32_t numBones);

private:
	core::MemoryArenaBase* arena_;

	int32_t numFrames_;
	int32_t fps_;

	core::Array<Bone> bones_;
};


X_NAMESPACE_END