#pragma once


#include <IAnimation.h>
#include <Util\UniquePointer.h>

#include <Time\TimeVal.h>

#include <Math\XQuatCompressed.h>

X_NAMESPACE_DECLARE(core, class MemCursor)

X_NAMESPACE_BEGIN(anim)

class Bone
{
public:
	Bone();

	void setName(const char* pName);

	void load(core::MemCursor& cursor);


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


class Anim : public IAnim
{
	X_NO_COPY(Anim);
	X_NO_ASSIGN(Anim);

	template<typename T>
	using AlignedArray = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 16>>;

	typedef AlignedArray<Matrix44f> Mat44Arr;

	typedef core::Array<Bone> BoneArr;

public:
	Anim(core::string& name, core::MemoryArenaBase* arena);
	~Anim() X_OVERRIDE;

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

	void processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data);

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