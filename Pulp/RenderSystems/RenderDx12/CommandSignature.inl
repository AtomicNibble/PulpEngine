
X_NAMESPACE_BEGIN(render)

IndirectParameter::IndirectParameter()
{
    indirectParam_.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF;
}

void IndirectParameter::draw(void)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
}

void IndirectParameter::drawIndexed(void)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
}

void IndirectParameter::dispatch(void)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
}

void IndirectParameter::vertexBufferView(uint32_t slot)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
    indirectParam_.VertexBuffer.Slot = slot;
}

void IndirectParameter::indexBufferView(void)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
}

void IndirectParameter::constant(uint32_t rootParameterIndex,
    uint32_t destOffsetIn32BitValues, uint32_t num32BitValuesToSet)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    indirectParam_.Constant.RootParameterIndex = rootParameterIndex;
    indirectParam_.Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
    indirectParam_.Constant.Num32BitValuesToSet = num32BitValuesToSet;
}

void IndirectParameter::constantBufferView(uint32_t rootParameterIndex)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
    indirectParam_.ConstantBufferView.RootParameterIndex = rootParameterIndex;
}

void IndirectParameter::shaderResourceView(uint32_t rootParameterIndex)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
    indirectParam_.ShaderResourceView.RootParameterIndex = rootParameterIndex;
}

void IndirectParameter::unorderedAccessView(uint32_t RootParameterIndex)
{
    indirectParam_.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
    indirectParam_.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
}

D3D12_INDIRECT_ARGUMENT_TYPE IndirectParameter::getType(void) const
{
    return indirectParam_.Type;
}

// ------------------------------------

X_INLINE IndirectParameter& CommandSignature::operator[](size_t idx)
{
    return params_[idx];
}

X_INLINE const IndirectParameter& CommandSignature::operator[](size_t idx) const
{
    return params_[idx];
}

X_INLINE ID3D12CommandSignature* CommandSignature::getSignature(void) const
{
    return pSignature_;
}

X_NAMESPACE_END