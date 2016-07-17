
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

		X_DELETE_ARRAY(pRange, arena_);
		arena_ = nullptr;
	}

	rootParam_.ParameterType = static_cast<D3D12_ROOT_PARAMETER_TYPE>(0xFFFFFFFF);
}

X_INLINE void RootParameter::initAsConstants(uint32_t Register, uint32_t NumDwords, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Constants.Num32BitValues = NumDwords;
	rootParam_.Constants.ShaderRegister = Register;
	rootParam_.Constants.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsConstantBuffer(uint32_t Register, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = Register;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsBufferSRV(uint32_t Register, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = Register;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsBufferUAV(uint32_t Register, D3D12_SHADER_VISIBILITY visibility)
{
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.Descriptor.ShaderRegister = Register;
	rootParam_.Descriptor.RegisterSpace = 0;
}

X_INLINE void RootParameter::initAsDescriptorRange(core::MemoryArenaBase* arena, D3D12_DESCRIPTOR_RANGE_TYPE Type, uint32_t Register,
	uint32_t Count, D3D12_SHADER_VISIBILITY visibility)
{
	initAsDescriptorTable(arena, 1, visibility);
	setTableRange(0, Type, Register, Count);
}

X_INLINE void RootParameter::initAsDescriptorTable(core::MemoryArenaBase* arena, uint32_t rangeCount, D3D12_SHADER_VISIBILITY visibility)
{
	X_ASSERT(arena_ == nullptr, "Arena must already be null")();
	X_ASSERT_NOT_NULL(arena);

	arena_ = arena;
	rootParam_.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam_.ShaderVisibility = visibility;
	rootParam_.DescriptorTable.NumDescriptorRanges = rangeCount;
	rootParam_.DescriptorTable.pDescriptorRanges = X_NEW_ARRAY(D3D12_DESCRIPTOR_RANGE, rangeCount, arena, "DescriptorTable");
}

X_INLINE void RootParameter::setTableRange(uint32_t rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type,
	uint32_t Register, uint32_t count, uint32_t space)
{
	D3D12_DESCRIPTOR_RANGE* pRange = const_cast<D3D12_DESCRIPTOR_RANGE*>(rootParam_.DescriptorTable.pDescriptorRanges + rangeIndex);
	pRange->RangeType = Type;
	pRange->NumDescriptors = count;
	pRange->BaseShaderRegister = Register;
	pRange->RegisterSpace = space;
	pRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
}

X_INLINE const D3D12_ROOT_PARAMETER& RootParameter::operator() (void) const
{ 
	return rootParam_;
}


// -----------------------------------------------------------------

X_INLINE RootSignature::RootSignature(core::MemoryArenaBase* arena, size_t numRootParams, size_t numStaticSamplers) :
	params_(arena),
	samplers_(arena),
	samplesInitCount_(0),
	pSignature_(nullptr)
{
	reset(numRootParams, numStaticSamplers);
}

X_INLINE RootSignature::~RootSignature()
{
	free();
}


X_INLINE void RootSignature::reset(size_t numRootParams, size_t numStaticSamplers)
{
	params_.resize(numRootParams);
	samplers_.resize(numStaticSamplers);

	samplesInitCount_ = 0;
}



X_INLINE size_t RootSignature::numParams(void) const
{
	return params_.size();
}

X_INLINE RootParameter& RootSignature::operator[] (size_t idx)
{
	return params_[idx];
}

X_INLINE const RootParameter& RootSignature::operator[] (size_t idx) const
{
	return params_[idx];
}

X_INLINE ID3D12RootSignature* RootSignature::getSignature(void) const
{
	return pSignature_;
}

X_INLINE uint32_t RootSignature::descriptorTableBitMap(void) const
{
	return descriptorTableBitMap_;
}

X_INLINE uint32_t RootSignature::descriptorTableSize(size_t idx) const
{
	return descriptorTableSize_[idx];
}

X_NAMESPACE_END