#pragma once

#include <IAnimation.h>
#include <IAsyncLoad.h>

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

	void setMatrixForFrame(Matrix44f& mat, int32_t frame) const;

private:

	Vec3f GetPostion(int32_t idx) const
	{
		Vec3<uint8_t>& scale = pPosScalers_[idx];

		Vec3f pos;
		pos[0] = (posMin_.x + (posRange_.x * (scale[0] / 255.f)));
		pos[1] = (posMin_.y + (posRange_.y * (scale[1] / 255.f)));
		pos[2] = (posMin_.z + (posRange_.z * (scale[2] / 255.f)));

		return pos;
	}

private:
	const char* pName_;

	int32_t numAngles_;
	int32_t numPos_;

	// Pos data
	Vec3f posMin_;
	Vec3f posRange_;
	Vec3<uint8_t>* pPosScalers_;
	uint8_t* pPosFrames_;

	// angle data.
	uint8_t* pAngleFrames_;
	XQuatCompressedf* pAngleData_;
};


class Anim 
{
	X_NO_COPY(Anim);
	X_NO_ASSIGN(Anim);

	template<typename T>
	using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

	typedef AlignedArray<Matrix44f> Mat44Arr;

	typedef core::Array<Bone> BoneArr;

public:
	ANIMLIB_EXPORT Anim(core::string& name, core::MemoryArenaBase* arena);
	ANIMLIB_EXPORT ~Anim();

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE core::LoadStatus::Enum getStatus(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool loadFailed(void) const;
	X_INLINE void setStatus(core::LoadStatus::Enum status);

	X_INLINE const core::string& getName(void) const;
	X_INLINE int32_t numBones(void) const;
	X_INLINE int32_t numFrames(void) const;
	X_INLINE int32_t fps(void) const;
	X_INLINE AnimType::Enum type(void) const;
	X_INLINE bool isLooping(void) const;
	X_INLINE bool hasNotes(void) const;

	void update(core::TimeVal delta, AnimState& state, Mat44Arr& bonesOut) const;

	ANIMLIB_EXPORT void processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data);

private:
	int32_t id_;
	core::string name_;

	core::LoadStatus::Enum status_;
	uint8_t _pad[3];

	BoneArr bones_;

	core::UniquePointer<uint8_t[]> data_;
	AnimHeader hdr_;
};


X_NAMESPACE_END

#include "Anim.inl"