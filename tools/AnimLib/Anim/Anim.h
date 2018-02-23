#pragma once

#include <IAnimation.h>
#include <Assets\AssetBase.h>

#include <Time\TimeVal.h>
#include <Util\UniquePointer.h>
#include <Math\XQuatCompressed.h>

X_NAMESPACE_DECLARE(core, class MemCursor)

X_NAMESPACE_BEGIN(anim)

class Bone
{
public:
	Bone();

	void setName(const char* pName);
	const char* getName(void) const {
		return pName_;
	}

	void load(core::MemCursor& cursor);

	void decodeFrame(Transformf& trans, int32_t frame) const;

private:
	void decodePos(Vec3f& pos, int32_t frame) const;
	void decodeAngle(Quatf& angle, int32_t frame) const;


private:
	Quatf GetAngle(int32_t idx) const;
	Vec3f GetPostion(int32_t idx) const;

private:
	const char* pName_;

	BoneFlags flags_;
	int32_t numAngles_;
	int32_t numPos_;

	// Pos data
	Vec3f posMin_;
	Vec3f posRange_;
	union {
		Vec3<uint8_t>* pPosScalers_;
		Vec3<uint16_t>* pPosScalersLarge_;
	};
	uint8_t* pPosFrames_;

	// angle data.
	uint8_t* pAngleFrames_;
	XQuatCompressedf* pAngleData_;
};


class Anim : public core::AssetBase
{
	X_NO_COPY(Anim);
	X_NO_ASSIGN(Anim);

	template<typename T>
	using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

	typedef AlignedArray<Matrix44f> Mat44Arr;

	typedef core::Array<Bone> BoneArr;
	typedef core::Array<Transformf> TransformArr;

	struct BoneIndexPair
	{
		uint8_t srcIdx;
		uint8_t dstIdx;
	};

	typedef core::Array<int32_t> IndexArr;
	typedef core::Array<Transformf> TransformArr;

public:
	ANIMLIB_EXPORT Anim(core::string& name, core::MemoryArenaBase* arena);
	ANIMLIB_EXPORT ~Anim();

	X_INLINE int32_t getNumBones(void) const;
	X_INLINE int32_t getNumFrames(void) const;
	X_INLINE int32_t getFps(void) const;
	X_INLINE int32_t getNumNotes(void) const;
	X_INLINE AnimType::Enum type(void) const;
	X_INLINE bool isLooping(void) const;
	X_INLINE bool hasNotes(void) const;
	X_INLINE core::TimeVal getDuration(void) const;

	X_INLINE const char* getBoneName(int32_t idx) const;
	X_INLINE int32_t getNoteFrame(int32_t idx) const;
	X_INLINE const char* getNoteValue(int32_t idx) const;


	void timeToFrame(core::TimeVal time, int32_t cycles, FrameBlend& frame) const;
	void getFrame(const FrameBlend& frame, TransformArr& boneTransOut, const IndexArr& indexes) const;
	void getOrigin(Vec3f& offset, core::TimeVal time, int32_t cycles) const;

	void getNotes(int32_t from, int32_t to, NoteTrackValueArr& values) const;

	ANIMLIB_EXPORT bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

private:
	X_INLINE const Note* getNote(int32_t idx) const;
	X_INLINE const Note* getNotes(void) const;
	X_INLINE const char* getNoteStrData(void) const;


private:
	BoneArr bones_;

	core::UniquePointer<char[]> data_;
	NoteTrackHdr* pNoteHdr_;
	AnimHeader* pHdr_;
};


X_NAMESPACE_END

#include "Anim.inl"