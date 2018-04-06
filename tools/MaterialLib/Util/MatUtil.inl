
X_NAMESPACE_BEGIN(engine)

namespace Util
{
    X_INLINE MaterialMountType::Enum MatMountTypeFromStr(const char* str)
    {
        return MatMountTypeFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE MaterialCat::Enum MatCatFromStr(const char* str)
    {
        return MatCatFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE MaterialUsage::Enum MatUsageFromStr(const char* str)
    {
        return MatUsageFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str)
    {
        return MatSurfaceTypeFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str)
    {
        return MatPolyOffsetFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::FilterType::Enum FilterTypeFromStr(const char* str)
    {
        return FilterTypeFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::TexRepeat::Enum TexRepeatFromStr(const char* str)
    {
        return TexRepeatFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::CullType::Enum CullTypeFromStr(const char* str)
    {
        return CullTypeFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::BlendType::Enum BlendTypeFromStr(const char* str)
    {
        return BlendTypeFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::BlendOp::Enum BlendOpFromStr(const char* str)
    {
        return BlendOpFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::StencilOperation::Enum StencilOpFromStr(const char* str)
    {
        return StencilOpFromStr(str, str + core::strUtil::strlen(str));
    }

    X_INLINE render::StencilFunc::Enum StencilFuncFromStr(const char* str)
    {
        return StencilFuncFromStr(str, str + core::strUtil::strlen(str));
    }

} // namespace Util

X_NAMESPACE_END