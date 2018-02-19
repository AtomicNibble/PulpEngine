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
	Matrix33f rotation;
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

struct InterNote
{
	core::string value;
	int32_t frame;
};

class InterAnim
{
public:
	typedef core::Array<InterNote> NoteArr;
	typedef core::Array<Bone> BoneArr;

public:
	InterAnim(core::MemoryArenaBase* arena);
	~InterAnim() = default;

	bool load(core::Path<char>& file);
	bool load(core::Path<wchar_t>& file);
	bool load(const core::Array<uint8_t>& fileData);
	bool load(const core::ByteStream& fileData);

	int32_t getNumFrames(void) const;
	int32_t getFps(void) const;
	size_t getNumBones(void) const;
	const Bone& getBone(size_t idx) const;
	const NoteArr& getNotes(void) const;

private:
	bool ParseData(core::XLexer& lex);
	static bool ReadheaderToken(core::XLexer& lex, const char* pName, int32_t& valOut, bool optional);
	bool ReadNotes(core::XLexer& lex, int32_t numNotes);
	bool ReadBones(core::XLexer& lex, int32_t numBones);
	bool ReadFrameData(core::XLexer& lex, int32_t numBones);

private:
	core::MemoryArenaBase* arena_;

	int32_t numFrames_;
	int32_t fps_;

	BoneArr bones_;
	NoteArr notes_;
};


X_NAMESPACE_END