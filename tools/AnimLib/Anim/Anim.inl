
X_NAMESPACE_BEGIN(anim)

X_INLINE int32_t Anim::getNumBones(void) const
{
	return pHdr_->numBones;
}

X_INLINE int32_t Anim::getNumFrames(void) const
{
	return pHdr_->numFrames;
}

X_INLINE int32_t Anim::getFps(void) const
{
	return pHdr_->fps;
}

X_INLINE int32_t Anim::getNumNotes(void) const
{
	return noteHdr_.num;
}

X_INLINE AnimType::Enum Anim::type(void) const
{
	return pHdr_->type;
}

X_INLINE bool Anim::isLooping(void) const
{
	return pHdr_->flags.IsSet(AnimFlag::LOOP);
}

X_INLINE bool Anim::hasNotes(void) const
{
	return pHdr_->flags.IsSet(AnimFlag::NOTES);
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
