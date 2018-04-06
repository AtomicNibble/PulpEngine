
X_NAMESPACE_BEGIN(render)

X_INLINE PSO::PSO()
{
    pRootSignature_ = nullptr;
    pPSO_ = nullptr;
}

X_INLINE PSO::~PSO()
{
}

X_INLINE bool PSO::isRootSigSet(void) const
{
    return pRootSignature_ != 0;
}

X_INLINE bool PSO::isDeviceObjectValid(void) const
{
    return pPSO_ != 0;
}

X_INLINE const RootSignature& PSO::getRootSignature(void) const
{
    X_ASSERT_NOT_NULL(pRootSignature_);
    return *pRootSignature_;
}

X_INLINE ID3D12PipelineState* PSO::getPipelineStateObject(void) const
{
    return pPSO_;
}

// ----------------------------------------

X_INLINE GraphicsPSO::GraphicsPSO()
{
    core::zero_object(PSODesc_);
    PSODesc_.NodeMask = 1;
    PSODesc_.SampleMask = 0xFFFFFFFFu;
    PSODesc_.SampleDesc.Count = 1;
    PSODesc_.InputLayout.NumElements = 0;
}

X_INLINE GraphicsPSO::~GraphicsPSO()
{
}

X_INLINE void GraphicsPSO::setBlendState(const D3D12_BLEND_DESC& blendDesc)
{
    PSODesc_.BlendState = blendDesc;
}

X_INLINE void GraphicsPSO::setRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
    PSODesc_.RasterizerState = rasterizerDesc;
}

X_INLINE void GraphicsPSO::setDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
    PSODesc_.DepthStencilState = depthStencilDesc;
}

X_INLINE void GraphicsPSO::setSampleMask(uint32_t sampleMask)
{
    PSODesc_.SampleMask = sampleMask;
}

X_INLINE void GraphicsPSO::setPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    PSODesc_.PrimitiveTopologyType = topologyType;
}

X_INLINE void GraphicsPSO::setRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat,
    uint32_t msaaCount, uint32_t msaaQuality)
{
    setRenderTargetFormats(1, &RTVFormat, DSVFormat, msaaCount, msaaQuality);
}

X_INLINE void GraphicsPSO::setPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
    PSODesc_.IBStripCutValue = IBProps;
}

X_INLINE void GraphicsPSO::setVertexShader(const void* pBinary, size_t Size)
{
    PSODesc_.VS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

X_INLINE void GraphicsPSO::setPixelShader(const void* pBinary, size_t Size)
{
    PSODesc_.PS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

X_INLINE void GraphicsPSO::setGeometryShader(const void* pBinary, size_t Size)
{
    PSODesc_.GS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

X_INLINE void GraphicsPSO::setHullShader(const void* pBinary, size_t Size)
{
    PSODesc_.HS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

X_INLINE void GraphicsPSO::setDomainShader(const void* pBinary, size_t Size)
{
    PSODesc_.DS = CD3D12_SHADER_BYTECODE(pBinary, Size);
}

X_INLINE void GraphicsPSO::setVertexShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.VS = binary;
}

X_INLINE void GraphicsPSO::setPixelShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.PS = binary;
}

X_INLINE void GraphicsPSO::setGeometryShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.GS = binary;
}

X_INLINE void GraphicsPSO::setHullShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.HS = binary;
}

X_INLINE void GraphicsPSO::setDomainShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.DS = binary;
}

// ----------------------------------------

X_INLINE ComputePSO::ComputePSO()
{
    core::zero_object(PSODesc_);
    PSODesc_.NodeMask = 1;
}

X_INLINE ComputePSO::~ComputePSO()
{
}

X_INLINE void ComputePSO::setComputeShader(const D3D12_SHADER_BYTECODE& binary)
{
    PSODesc_.CS = binary;
}

X_NAMESPACE_END