#pragma once


#include <IAnimation.h>
#include <Util\UniquePointer.h>


X_NAMESPACE_BEGIN(anim)


class Anim : public IAnim
{
	X_NO_COPY(Anim);
	X_NO_ASSIGN(Anim);

public:
	Anim(core::string& name);
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


	void processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data);

private:
	int32_t id_;
	core::string name_;

	core::LoadStatus::Enum status_;
	uint8_t _pad[3];

	core::UniquePointer<uint8_t[]> data_;
	AnimHeader hdr_;
};


X_NAMESPACE_END

#include "Anim.inl"