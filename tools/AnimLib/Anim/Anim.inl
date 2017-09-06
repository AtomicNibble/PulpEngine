
X_NAMESPACE_BEGIN(anim)



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

X_INLINE int32_t Anim::getNumBones(void) const
{
	return hdr_.numBones;
}

X_INLINE int32_t Anim::getNumFrames(void) const
{
	return hdr_.numFrames;
}

X_INLINE int32_t Anim::getFps(void) const
{
	return hdr_.fps;
}

X_INLINE AnimType::Enum Anim::type(void) const
{
	return hdr_.type;
}

X_INLINE bool Anim::isLooping(void) const
{
	return hdr_.flags.IsSet(AnimFlag::LOOP);
}

X_INLINE bool Anim::hasNotes(void) const
{
	return hdr_.flags.IsSet(AnimFlag::NOTES);
}

X_INLINE core::TimeVal Anim::getDuration(void) const
{
	core::TimeVal oneSecond;
	oneSecond.SetSeconds(1ll);

	return core::TimeVal((getNumFrames() * oneSecond.GetValue()) / getFps());
}

X_INLINE const char* Anim::getBoneName(int32_t idx) const
{
	return bones_[idx].getName();
}

X_NAMESPACE_END
