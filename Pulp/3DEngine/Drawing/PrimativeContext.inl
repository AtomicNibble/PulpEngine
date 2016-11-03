

X_NAMESPACE_BEGIN(engine)


X_INLINE PrimativeContext::PushBufferEntry::PushBufferEntry(uint32 numVertices, uint32 vertexOffs,
	PrimRenderFlags flags) :
	numVertices(numVertices),
	vertexOffs(vertexOffs),
	flags(flags)
{

}

// --------------------------------------------------------

X_INLINE PrimativeContext::PrimRenderFlags::PrimRenderFlags(PrimitiveType::Enum type, texture::TexID textureId) :
	flags_((type << PrimFlagBitMasks::PrimTypeShift) | (textureId << PrimFlagBitMasks::TextureIdShift))
{
	static_assert(PrimitiveType::ENUM_COUNT < ((1 << PrimFlagBitMasks::PrimTypeBits) - 1), "Not enougth bits to store primative type");
//	static_assert(std::numeric_limits<decltype(textureId)>::max() < ((1 << PrimFlagBitMasks::TextureIdBits) - 1), "Not enougth bits to store texture id");
}

X_INLINE bool PrimativeContext::PrimRenderFlags::operator ==(const PrimRenderFlags& rhs) const
{
	return flags_ == rhs.flags_;
}

X_INLINE bool PrimativeContext::PrimRenderFlags::operator !=(const PrimRenderFlags& rhs) const
{
	return flags_ != rhs.flags_;
}

X_INLINE bool PrimativeContext::PrimRenderFlags::operator <(const PrimRenderFlags& rhs) const
{
	return flags_ < rhs.flags_;
}

X_INLINE PrimativeContext::PrimitiveType::Enum PrimativeContext::PrimRenderFlags::getPrimType(void) const
{
	return static_cast<PrimitiveType::Enum>((flags_ & PrimFlagBitMasks::PrimTypeMask) >> PrimFlagBitMasks::PrimTypeShift);
}

X_INLINE texture::TexID PrimativeContext::PrimRenderFlags::getTextureId(void) const
{
	return static_cast<texture::TexID>((flags_ & PrimFlagBitMasks::TextureIdMask) >> PrimFlagBitMasks::TextureIdShift);
}

X_INLINE uint32_t PrimativeContext::PrimRenderFlags::toInt(void) const
{
	return flags_;
}

X_NAMESPACE_END