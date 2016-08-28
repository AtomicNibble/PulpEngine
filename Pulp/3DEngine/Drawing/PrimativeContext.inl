

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
	flags((type << PrimFlagBitMasks::PrimTypeShift) | (textureId << PrimFlagBitMasks::TextureIdShift))
{

}

X_INLINE bool PrimativeContext::PrimRenderFlags::operator ==(const PrimRenderFlags& rhs) const
{
	return flags == rhs.flags;
}

X_INLINE bool PrimativeContext::PrimRenderFlags::operator !=(const PrimRenderFlags& rhs) const
{
	return flags == rhs.flags;
}

X_INLINE PrimativeContext::PrimRenderFlags::operator uint32_t()
{
	return flags;
}

X_INLINE PrimativeContext::PrimRenderFlags::operator const uint32_t() const
{
	return flags;
}



X_NAMESPACE_END