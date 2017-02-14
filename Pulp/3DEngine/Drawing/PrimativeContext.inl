

X_NAMESPACE_BEGIN(engine)



X_INLINE Material* PrimativeContextSharedResources::getMaterial(PrimitiveType::Enum prim) const
{
	return primMaterials_[prim];
}

X_INLINE Material* PrimativeContextSharedResources::getMaterialDepthTest(PrimitiveType::Enum prim) const
{
	return primMaterialsDepth_[prim];
}

// --------------------------------------------------------


X_INLINE PrimativeContext::PushBufferEntry::PushBufferEntry(uint16 numVertices, uint16 vertexOffs, int32_t pageIdx,
	Material* pMaterial) :
	numVertices(numVertices),
	vertexOffs(vertexOffs),
	material(pMaterial)
{
	material.SetBits(pageIdx);
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


	X_ASSERT(numVerts <= NUMVERTS_PER_PAGE, "Vert page exceeded it's limits")(numVerts, NUMVERTS_PER_PAGE);

	// we need to give the render system a 16byte aligned buffer that is a multiple of 16 bytes.
	// i think i might support not requiring the buffer size to be a multiple of 16 as that means we always need padding if vert not multiple of 16.
	return core::bitUtil::RoundUpToMultiple<uint32_t>(numVerts * sizeof(PrimVertex), 16u);
}

X_INLINE const uint32_t PrimativeContext::VertexPage::freeSpace(void) const
{
	return NUMVERTS_PER_PAGE - safe_static_cast<uint32_t>(verts.size());
}


// --------------------------------------------------------


X_NAMESPACE_END