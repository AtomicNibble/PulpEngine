#include "stdafx.h"
#include "ShaderUtil.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{
    namespace Util
    {
        namespace
        {
            // GL = global
            // PB = Per-Batch
            // PI = Per-Instance
            // SI = Per-Instance Static
            // PF = Per-Frame
            // PM = Per-Material
            // SK = Skin data
            static const XParamDB g_SematicParams[] = {

                XParamDB("worldToScreenMatrix", ParamType::PF_worldToScreenMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),
                XParamDB("screenToWorldMatrix", ParamType::PF_screenToWorldMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),
                XParamDB("worldToCameraMatrix", ParamType::PF_worldToCameraMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),
                XParamDB("cameraToWorldMatrix", ParamType::PF_cameraToWorldMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),
                XParamDB("viewMatrix", ParamType::PF_viewMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),
                XParamDB("projectionMatrix", ParamType::PF_projectionMatrix, UpdateFreq::FRAME, ParamFlag::MATRIX),

                XParamDB("time", ParamType::PF_Time, UpdateFreq::FRAME, ParamFlag::FLOAT),
                XParamDB("frameTime", ParamType::PF_FrameTime, UpdateFreq::FRAME, ParamFlag::FLOAT),
                XParamDB("frameTimeUI", ParamType::PF_FrameTimeUI, UpdateFreq::FRAME, ParamFlag::FLOAT),
                XParamDB("screensize", ParamType::PF_ScreenSize, UpdateFreq::FRAME, ParamFlag::VEC4),
                XParamDB("cameraPos", ParamType::PF_CameraPos, UpdateFreq::FRAME, ParamFlag::VEC4),

                XParamDB("objectToWorldMatrix", ParamType::PI_objectToWorldMatrix, UpdateFreq::INSTANCE, ParamFlag::MATRIX),
                XParamDB("worldMatrix", ParamType::PI_worldMatrix, UpdateFreq::INSTANCE, ParamFlag::MATRIX),
                XParamDB("worldViewProjectionMatrix", ParamType::PI_worldViewProjectionMatrix, UpdateFreq::INSTANCE, ParamFlag::MATRIX),
            };

        } // namespace

        XParamDB::XParamDB(const char* pName_, ParamType::Enum type_, UpdateFreq::Enum ur) :
            XParamDB(pName_, type_, ur, ParamFlags())
        {
        }

        XParamDB::XParamDB(const char* pName_, ParamType::Enum type_, UpdateFreq::Enum ur, ParamFlags flags_)
        {
            core::StackString<192, char> nameUpper(pName_);
            nameUpper.toUpper();

            pName = pName_;
            upperNameHash = core::StrHash(nameUpper.c_str(), nameUpper.length());
            type = type_;
            updateRate = ur;
            flags = flags_;
        }

        const XParamDB* getPredefinedParamForName(const char* pName)
        {
            static_assert(ParamType::FLAGS_COUNT == 15, "ParamType count changed, check if this code needs updating");

            const size_t num = sizeof(g_SematicParams) / sizeof(XParamDB);

            core::StackString<192, char> nameUpper(pName);
            nameUpper.toUpper();
            core::StrHash upperNameHash(nameUpper.c_str(), nameUpper.length());

            for (size_t i = 0; i < num; i++) {
                const auto& param = g_SematicParams[i];

                if (upperNameHash != param.upperNameHash) {
                    continue;
                }

                if (core::strUtil::IsEqualCaseInsen(pName, param.pName)) {
                    return &param;
                }
            }

            return nullptr;
        }

        // -------------------------------------------------------------------------

        InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt)
        {
            static_assert(VertexFormat::ENUM_COUNT == 10, "Added vertex formats? this code needs updating.");

            switch (fmt) {
                case VertexFormat::P3F_T3F:
                    return InputLayoutFormat::POS_UV;
                case VertexFormat::P3F_T2S:
                    return InputLayoutFormat::POS_UV;

                case VertexFormat::P3F_T2S_C4B:
                    return InputLayoutFormat::POS_UV_COL;
                case VertexFormat::P3F_T2S_C4B_N3F:
                    return InputLayoutFormat::POS_UV_COL_NORM;
                case VertexFormat::P3F_T2S_C4B_N3F_TB3F:
                    return InputLayoutFormat::POS_UV_COL_NORM_TAN_BI;

                case VertexFormat::P3F_T2S_C4B_N10:
                    return InputLayoutFormat::POS_UV_COL_NORM;
                case VertexFormat::P3F_T2S_C4B_N10_TB10:
                    return InputLayoutFormat::POS_UV_COL_NORM_TAN_BI;

                case VertexFormat::P3F_T2F_C4B:
                    return InputLayoutFormat::POS_UV_COL;

                case VertexFormat::P3F_T4F_C4B_N3F:
                    return InputLayoutFormat::POS_UV2_COL_NORM;

                case VertexFormat::NONE:
                    return InputLayoutFormat::NONE;

                default:
#if X_DEBUG
                    X_ASSERT_UNREACHABLE();
                    return InputLayoutFormat::Invalid;
#else
                    X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
            }
        }

        ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt)
        {
            static_assert(VertexFormat::ENUM_COUNT == 10, "Added vertex formats? this code needs updating.");

            switch (fmt) {
                case VertexFormat::P3F_T3F:
                    return ILFlags();
                case VertexFormat::P3F_T2S:
                    return ILFlags();

                case VertexFormat::P3F_T2S_C4B:
                    return ILFlag::Color;
                case VertexFormat::P3F_T2S_C4B_N3F:
                    return ILFlag::Color | ILFlag::Normal;
                case VertexFormat::P3F_T2S_C4B_N3F_TB3F:
                    return ILFlag::Color | ILFlag::Normal | ILFlag::BiNormal;

                case VertexFormat::P3F_T2S_C4B_N10:
                    return ILFlag::Color | ILFlag::Normal;
                case VertexFormat::P3F_T2S_C4B_N10_TB10:
                    return ILFlag::Color | ILFlag::Normal | ILFlag::BiNormal;

                case VertexFormat::P3F_T2F_C4B:
                    return ILFlag::Color;

                case VertexFormat::P3F_T4F_C4B_N3F:
                    return ILFlag::Color | ILFlag::Normal | ILFlag::Uv2;

                case VertexFormat::NONE:
                    return ILFlags();

                default:
#if X_DEBUG
                    X_ASSERT_UNREACHABLE();
                    return ILFlags();
#else
                    X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
            }
        }

        const char* getProfileFromType(ShaderType::Enum type)
        {
            switch (type) {
                case ShaderType::Vertex:
                    return "vs_5_0";
                case ShaderType::Pixel:
                    return "ps_5_0";
                case ShaderType::Geometry:
                    return "gs_5_0";
                case ShaderType::Hull:
                    return "hs_5_0";
                case ShaderType::Domain:
                    return "ds_5_0";

                case ShaderType::UnKnown:
                    break;
            }

            X_ASSERT_UNREACHABLE();
            return "";
        }

        std::pair<uint8_t, uint8_t> getProfileVersionForType(ShaderType::Enum type)
        {
            uint8_t major, minor;

            major = 0;
            minor = 0;

            switch (type) {
                case ShaderType::Vertex:
                case ShaderType::Pixel:
                case ShaderType::Geometry:
                case ShaderType::Hull:
                case ShaderType::Domain:
                    major = 5;
                    minor = 0;
                    break;

                default:
                    X_ASSERT_UNREACHABLE();
                    break;
            }

            return {major, minor};
        }

        ParamFlags VarTypeToFlags(const D3D12_SHADER_TYPE_DESC& CDesc)
        {
            ParamFlags f;

            if (CDesc.Class == D3D_SVC_MATRIX_COLUMNS) {
                f.Set(ParamFlag::MATRIX);
            }
            else if (CDesc.Class == D3D_SVC_VECTOR) {
                if (CDesc.Columns == 4) {
                    f.Set(ParamFlag::VEC4);
                }
                else if (CDesc.Columns == 3) {
                    f.Set(ParamFlag::VEC3);
                }
                else if (CDesc.Columns == 2) {
                    f.Set(ParamFlag::VEC2);
                }
                else {
                    X_ASSERT_NOT_IMPLEMENTED();
                }
            }
            else {
                X_ASSERT_NOT_IMPLEMENTED();
            }

            switch (CDesc.Type) {
                case D3D_SVT_BOOL:
                    f.Set(ParamFlag::BOOL);
                    break;
                case D3D_SVT_INT:
                    f.Set(ParamFlag::INT);
                    break;
                case D3D_SVT_FLOAT:
                    f.Set(ParamFlag::FLOAT);
                    break;
            }

            return f;
        }

    } // namespace Util

} // namespace shader

X_NAMESPACE_END