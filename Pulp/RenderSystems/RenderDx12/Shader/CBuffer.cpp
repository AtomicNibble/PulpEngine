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

	bool XShaderParam::isEqual(const XShaderParam& oth) const
	{
		if (nameHash != oth.nameHash) {
			return false;
		}

		if (name != oth.name) {
			return false;
		}

		if (flags.ToInt() != oth.flags.ToInt() || 
			type != oth.type || 
			slot != oth.slot || 
			bind != oth.bind || 
			numParameters != oth.numParameters) {
			return false;
		}

		return true;
	}

	// -------------------------------------------------------------------


	XCBuffer::XCBuffer(core::MemoryArenaBase* arena) :
		size(0),
		allParamsPreDefined(false),
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

	bool XCBuffer::isEqual(const XCBuffer& oth) const
	{
		if (size != oth.size) {
			return false;
		}
		if (name != oth.name) {
			return false;
		}

		// now check all the params are the smae.
		if (params.size() != oth.params.size()) {
			X_WARNING("CBuffer", "buffer has smae name but diffrent params");
			return false;
		}

		for (size_t i = 0; i < params.size(); i++)
		{
			if (!params[i].isEqual(oth.params[i])) {
				// i put this hear as I want to see the scenarios this happens.
				// as i potentially need to handle the fact some params may be marked as unused in one stage and not in others.
				X_WARNING("CBuffer", "buffer has smae name but diffrent params");
				return false;
			}
		}

		return true;
	}

	bool XCBuffer::fullyPreDefinedParams(void) const
	{
		return allParamsPreDefined;
	}

} // namespace shader

X_NAMESPACE_END