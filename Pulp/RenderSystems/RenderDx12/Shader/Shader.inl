

X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_INLINE ShaderID XShader::getID(void)
	{ 
		return XBaseAsset::getID(); 
	}

	X_INLINE const int32_t XShader::addRef(void)
	{ 
		return XBaseAsset::addRef(); 
	}

	X_INLINE const char* XShader::getName(void) const
	{ 
		return name_.c_str(); 
	}

	//X_INLINE VertexFormat::Enum XShader::getVertexFmt(void)
	//{ 
	//	return vertexFmt_; 
	//}


	X_INLINE size_t XShader::numTechs(void) const 
	{
		return techs_.size(); 
	}


} // namespace shader

X_NAMESPACE_END