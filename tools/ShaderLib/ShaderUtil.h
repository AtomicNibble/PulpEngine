#pragma once

#include <IShader.h>

#include <String\StringHash.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace Util
	{
		// we need a list of chicken ready for dippin!
		struct XParamDB
		{
			XParamDB(const char* pName, ParamType::Enum type, UpdateFreq::Enum ur);
			XParamDB(const char* pName, ParamType::Enum type, UpdateFreq::Enum ur, ParamFlags flags);

			const char* pName;
			core::StrHash upperNameHash;
			ParamType::Enum type;
			UpdateFreq::Enum updateRate;
			ParamFlags flags;
		};

		const XParamDB* getPredefinedParamForName(const char* pName);

		InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt);
		ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt);

		const char* getProfileFromType(ShaderType::Enum type);
		std::pair<uint8_t, uint8_t> getProfileVersionForType(ShaderType::Enum type);

		ParamFlags VarTypeToFlags(const D3D12_SHADER_TYPE_DESC& CDesc);
	}
}

X_NAMESPACE_END