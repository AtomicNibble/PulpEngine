

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

X_NAMESPACE_END