

X_NAMESPACE_BEGIN(engine)


X_INLINE PrimativeContext::PushBufferEntry::PushBufferEntry(uint16 numVertices, uint16 vertexOffs, int32_t pageIdx,
	PrimRenderFlags flags, render::StateHandle stateHandle) :
	numVertices(numVertices),
	vertexOffs(vertexOffs),
	pageIdx(pageIdx),
	flags(flags),
	stateHandle(stateHandle)
{

}

// --------------------------------------------------------

X_INLINE void PrimativeContext::VertexPage::reset(void)
{
	verts.clear();
}

X_INLINE bool PrimativeContext::VertexPage::isVbValid(void) const
{
	return vertexBufHandle != render::INVALID_BUF_HANLDE;
}

X_INLINE const uint32_t PrimativeContext::VertexPage::getVertBufBytes(void) const
{
	uint32_t numVerts = safe_static_cast<uint32_t>(verts.size());


	X_ASSERT(numVerts < NUMVERTS_PER_PAGE, "Vert page exceeded it's limits")(numVerts, NUMVERTS_PER_PAGE);

	// we need to give the render system a 16byte aligned buffer that is a multiple of 16 bytes.
	// i think i might support not requiring the buffer size to be a multiple of 16 as that means we always need padding if vert not multiple of 16.
	return core::bitUtil::RoundUpToMultiple<uint32_t>(numVerts * sizeof(PrimVertex), 16u);
}

X_INLINE const uint32_t PrimativeContext::VertexPage::freeSpace(void) const
{
	return NUMVERTS_PER_PAGE - safe_static_cast<uint32_t>(verts.size());
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