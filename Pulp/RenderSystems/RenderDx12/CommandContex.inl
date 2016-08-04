

X_NAMESPACE_BEGIN(render)


X_INLINE Param::Param(float32_t f) :
	fval(f)
{
}
X_INLINE Param::Param(uint32_t u) :
	uint(u)
{
}
X_INLINE Param::Param(int32_t i) :
	sint(i)
{
}

X_INLINE void Param::operator= (float32_t f)
{
	fval = f;
}
X_INLINE void Param::operator= (uint32_t u)
{
	uint = u;
}
X_INLINE void Param::operator= (int32_t i)
{
	sint = i;
}


// ----------------------------------


X_INLINE D3D12_COMMAND_LIST_TYPE CommandContext::getType(void) const
{
	return type_;
}

X_INLINE GraphicsContext& CommandContext::getGraphicsContext(void)
{
	X_ASSERT(type_ != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics")(type_);
	return reinterpret_cast<GraphicsContext&>(*this);
}

X_INLINE ComputeContext& CommandContext::getComputeContext(void)
{
	return reinterpret_cast<ComputeContext&>(*this);
}



template <uint32_t maxSubresources>
X_INLINE uint64_t CommandContext::updateSubresources(
	GpuResource& dest,
	ID3D12Resource* pIntermediate,
	uint64_t intermediateOffset,
	uint32_t firstSubresource,
	uint32_t numSubresources,
	D3D12_SUBRESOURCE_DATA* pSrcData)
{
	uint64_t requiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[maxSubresources];
	uint32_t numRows[maxSubresources];
	uint64_t rowSizesInBytes[maxSubresources];

	ID3D12Resource* pDestinationResource = dest.getResource();

	D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
	ID3D12Device* pDevice;
	pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, firstSubresource, numSubresources, intermediateOffset,
		layouts, numRows, rowSizesInBytes, &requiredSize);
	pDevice->Release();

	return updateSubresources(dest, pIntermediate, firstSubresource, numSubresources,
		requiredSize, layouts, numRows, rowSizesInBytes, pSrcData);
}




X_NAMESPACE_END