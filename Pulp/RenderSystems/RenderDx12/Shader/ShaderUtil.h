#pragma once

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace Util
	{

		InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt);
		ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt);

		const char* getProfileFromType(ShaderType::Enum type);
		std::pair<uint8_t, uint8_t> getProfileVersionForType(ShaderType::Enum type);

	}
}

X_NAMESPACE_END