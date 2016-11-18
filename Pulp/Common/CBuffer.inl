


X_NAMESPACE_BEGIN(render)

namespace shader
{


	X_INLINE bool XCBuffer::fullyPreDefinedParams(void) const
	{
		return allParamsPreDefined;
	}

	X_INLINE int16_t XCBuffer::getBindPoint(void) const
	{
		return bindPoint;
	}

	X_INLINE int16_t XCBuffer::getBindCount(void) const
	{
		return bindCount;
	}

} // namespace shader

X_NAMESPACE_END