
X_NAMESPACE_BEGIN(render)



X_INLINE RootParameter::RootParameter() 
{
	rootParam_.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(0xFFFFFFFF);
}

X_INLINE RootParameter::~RootParameter()
{
	clear();
}

X_INLINE void RootParameter::clear(void)
{
	if (rootParam_.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
		D3D12_DESCRIPTOR_RANGE* pRange = const_cast<D3D12_DESCRIPTOR_RANGE*>(rootParam_.DescriptorTable.pDescriptorRanges);

		X_DELETE_ARRAY(pRange, g_rendererArena);
	}

	rootParam_.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(0xFFFFFFFF);
}

X_INLINE void RootParameter::initAsConstants(uint32_t shaderRegister, uint32_t NumDwords, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Constants.Num32BitValues = NumDwords;
	rootParam_.Constants.ShaderRegister = shaderRegister;
	rootParam_.Constants.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsCBV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = shaderRegister;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsSRV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = shaderRegister;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsUAV(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = shaderRegister;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t baseShaderRegister,
	uint32_t count, D3D12_SHADER_VISIBILITY visibility)
{
	initAsDescriptorTable(1, visibility);
	setTableRange(0, type, baseShaderRegister, count);
}

X_INLINE void RootParameter::initAsDescriptorTable(uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.DescriptorTable.NumDescriptorRanges = rangeCount;
	rootParam_.DescriptorTable.pDescriptorRanges = X_NEW_ARRAY(D3D12_DESCRIPTOR_RANGE, rangeCount, g_rendererArena, "DescriptorTable");
}

X_INLINE void RootParameter::setTableRange(uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type,
	uint32_t baseShaderRegister, uint32_t count, uint32_t space)
{
	D3D12_DESCRIPTOR_RANGE* pRange = const_cast<D3D12_DESCRIPTOR_RANGE*>(rootParam_.DescriptorTable.pDescriptorRanges + rangeIndex);
	pRange->RangeType = Type;
	pRange->NumDescriptors = count;
	pRange->BaseShaderRegister = baseShaderRegister;
	pRange->RegisterSpace = space;
	pRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

X_INLINE const D3D12_ROOT_PARAMETER& RootParameter::operator() (void) const
{ 
	return rootParam_;
}

X_INLINE D3D12_ROOT_PARAMETER_TYPE RootParameter::getType(void) const
{
	return rootParam_.ParameterType;
}
// -----------------------------------------------------------------

X_INLINE RootSignature::RootSignature(core::MemoryArenaBase* arena, size_t numRootParams, size_t numStaticSamplers) :
	params_(arena),
	samplers_(arena),
	samplesInitCount_(0),
	pSignature_(nullptr)
{
	samplers_.setGranularity(1);

	// should be safe to leave these un-init, in release builds
	descriptorTableBitMap_ = 0;
	descriptorTableSamplerBitMap_ = 0;
	maxDescriptorCacheHandleCount_ = 0;
	core::zero_object(descriptorTableSize_);
	// ~

	reset(numRootParams, numStaticSamplers);
}

X_INLINE RootSignature::~RootSignature()
{
	freeParams();
}


X_INLINE void RootSignature::reset(size_t numRootParams, size_t numStaticSamplers)
{
	// since param is a min of 1 dword it's not possible to haver more than 64.
	X_ASSERT(numRootParams <= MAX_DWORDS, "num root params exceedes max")(numRootParams, MAX_DWORDS);

	params_.resize(numRootParams);
	samplers_.resize(numStaticSamplers);

	samplesInitCount_ = 0;
}



X_INLINE size_t RootSignature::numParams(void) const
{
	return params_.size();
}

X_INLINE RootParameter& RootSignature::getParamRef(size_t idx)
{
	return params_[idx];
}

X_INLINE const RootParameter& RootSignature::getParamRef(size_t idx) const
{
	return params_[idx];
}

X_INLINE ID3D12RootSignature* RootSignature::getSignature(void) const
{
	return pSignature_;
}

X_INLINE uint32_t RootSignature::descriptorTableBitMap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		return descriptorTableBitMap_;
	}
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) {
		return descriptorTableSamplerBitMap_;
	}

	X_ASSERT_UNREACHABLE();
	return 0;
}

X_INLINE uint32_t RootSignature::descriptorTableSize(size_t idx) const
{
	return descriptorTableSize_[idx];
}

X_NAMESPACE_END