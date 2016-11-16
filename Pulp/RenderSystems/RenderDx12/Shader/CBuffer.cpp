#include "stdafx.h"
#include "CBuffer.h"


X_NAMESPACE_BEGIN(render)

namespace shader
{



	XShaderParam::XShaderParam() :
		type(ParamType::Unknown),
		bind(-1),
		numParameters(0)
	{

	}

	void XShaderParam::print(void) const
	{
		X_LOG0("Param", "Name: \"%s\"", name.c_str());

	}

	// -------------------------------------------------------------------


	XCBuffer::XCBuffer(core::MemoryArenaBase* arena) :
		size(0),
		bindPoint(-1),
		bindCount(-1),
		params(arena)
	{

	}

	void XCBuffer::print(void) const
	{
		X_LOG0("CBuffer", "Name: \"%s\"", name.c_str());
		for (const auto& p : params)
		{
			p.print();
		}
	}



} // namespace shader

X_NAMESPACE_END