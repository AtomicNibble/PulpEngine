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



	void processData(AnimHeader& hdr, core::UniquePointer<uint8_t[]> data);

private:
	int32_t id_;
	core::string name_;

	core::LoadStatus::Enum status_;
	uint8_t _pad[3];

};


X_INLINE const int32_t Anim::getID(void) const
{
	return id_;
}

X_INLINE void Anim::setID(int32_t id)
{
	id_ = id;
}


X_INLINE core::LoadStatus::Enum Anim::getStatus(void) const
{
	return status_;
}

X_INLINE bool Anim::isLoaded(void) const
{
	return status_ == core::LoadStatus::Complete;
}

X_INLINE bool Anim::loadFailed(void) const
{
	return status_ == core::LoadStatus::Error;
}

X_INLINE void Anim::setStatus(core::LoadStatus::Enum status)
{
	status_ = status;
}

X_INLINE const core::string& Anim::getName(void) const
{
	return name_;
}


X_NAMESPACE_END