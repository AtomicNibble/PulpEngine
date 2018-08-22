

X_NAMESPACE_BEGIN(engine)

// --------------------------------------------------------

X_INLINE bool PrimativeContextSharedResources::InstancedPage::isVbValid(void) const
{
    return instBufHandle != render::INVALID_BUF_HANLDE;
}

// --------------------------------------------------------

X_INLINE Material* PrimativeContextSharedResources::getMaterial(MaterialSet::Enum set, PrimitiveType::Enum prim) const
{
    return primMaterials_[set][prim];
}

X_INLINE const PrimativeContextSharedResources::Shape& PrimativeContextSharedResources::getShapeResources(ShapeType::Enum shape) const
{
    return shapes_[shape];
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

X_INLINE void PrimativeContext::ShapeInstanceDataContainer::clear(void)
{
    data_.clear();
    core::zero_object(shapeCounts_);
}

X_INLINE bool PrimativeContext::ShapeInstanceDataContainer::isEmpty(void) const
{
    return data_.isEmpty();
}

X_INLINE size_t PrimativeContext::ShapeInstanceDataContainer::size(void) const
{
    return data_.size();
}

X_INLINE const PrimativeContext::ShapeInstanceDataContainer::ShapeCountArr& PrimativeContext::ShapeInstanceDataContainer::getShapeCounts(void) const
{
    return shapeCounts_;
}

X_INLINE PrimativeContext::ShapeInstanceData* PrimativeContext::ShapeInstanceDataContainer::addShape(bool solid, int32_t lodIdx)
{
    // we count how many of each type we have in the buffer
    // this way we know the index ranges of each group post sort.
    ++shapeCounts_[solid][lodIdx];

    ShapeInstanceData* pInstData = &data_.AddOne();
    pInstData->lodIdx = lodIdx;
    pInstData->solid = solid ? 1 : 0;
    return pInstData;
}

X_INLINE const PrimativeContext::ShapeParamArr& PrimativeContext::ShapeInstanceDataContainer::getData(void) const
{
    return data_;
}

X_NAMESPACE_END