
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
	if (!pNoteHdr_) {
		return 0;
	}

	return pNoteHdr_->num;
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

X_INLINE float Anim::getNotePosition(int32_t idx) const
{
	auto* pNote = getNote(idx);

	return pNote->frame;
}

X_INLINE const char* Anim::getNoteValue(int32_t idx) const
{
	auto* pNote = getNote(idx);
	auto stringOffset = pNote->value;

	const char* pValue = getNoteStrData();
	pValue += stringOffset;

	return pValue;
}

X_INLINE const Note* Anim::getNote(int32_t idx) const
{
	X_ASSERT(idx < pNoteHdr_->num, "Note idx out of range")(idx, pNoteHdr_->num);
	return getNotes() + idx;
}

X_INLINE const Note* Anim::getNotes(void) const
{
	X_ASSERT_NOT_NULL(pNoteHdr_);

	const Note* pNotes = reinterpret_cast<const Note*>(pNoteHdr_ + 1);
	return pNotes;
}


X_INLINE const char* Anim::getNoteStrData(void) const
{
	X_ASSERT_NOT_NULL(pNoteHdr_);

	const Note* pNotes = reinterpret_cast<const Note*>(pNoteHdr_ + 1);
	const char* pStrData = reinterpret_cast<const char*>(pNotes + pNoteHdr_->num);

	return pStrData;
}

X_NAMESPACE_END
