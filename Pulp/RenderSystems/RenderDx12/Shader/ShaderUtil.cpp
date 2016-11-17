#include "stdafx.h"
#include "ShaderUtil.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace Util
	{

		InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt)
		{
			switch (fmt)
			{
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
			switch (fmt)
			{
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
					return ILFlag::Color | ILFlag::Normal;

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
			switch (type)
			{
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

			switch (type)
			{
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

			return { major, minor };
		}

	} // namespace Util
} // namespace shader

X_NAMESPACE_END