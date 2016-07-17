

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

X_NAMESPACE_END