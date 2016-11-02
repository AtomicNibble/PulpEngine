#pragma once

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace Util
	{

		InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt);
		ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt);

	}
}

X_NAMESPACE_END