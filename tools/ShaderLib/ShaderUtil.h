#pragma once

#include <IShader.h>

#include <String\StringHash.h>

struct _D3D12_SHADER_TYPE_DESC;
typedef _D3D12_SHADER_TYPE_DESC D3D12_SHADER_TYPE_DESC;

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

        SHADERLIB_EXPORT const XParamDB* getPredefinedParamForName(const char* pName);

        SHADERLIB_EXPORT InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt);
        SHADERLIB_EXPORT ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt);

        SHADERLIB_EXPORT const char* getProfileFromType(ShaderType::Enum type);
        SHADERLIB_EXPORT std::pair<uint8_t, uint8_t> getProfileVersionForType(ShaderType::Enum type);

        SHADERLIB_EXPORT ParamFlags VarTypeToFlags(const D3D12_SHADER_TYPE_DESC& CDesc);
    } // namespace Util
} // namespace shader

X_NAMESPACE_END