
X_NAMESPACE_BEGIN(render)

X_INLINE PSO::PSO()
{

}

X_INLINE PSO::~PSO()
{

}


X_INLINE ID3D12PipelineState* PSO::getPipelineStateObject(void) const
{
	return pPSO_;
}

// ----------------------------------------


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




X_NAMESPACE_END