#pragma once

#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)

namespace Util
{
    MATLIB_EXPORT Register::Enum RegisterSlotFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT MaterialMountType::Enum MatMountTypeFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT MaterialCat::Enum MatCatFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT MaterialUsage::Enum MatUsageFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT MaterialSurType::Enum MatSurfaceTypeFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::FilterType::Enum FilterTypeFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::TexRepeat::Enum TexRepeatFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::CullType::Enum CullTypeFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::BlendType::Enum BlendTypeFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::BlendOp::Enum BlendOpFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::WriteMaskFlags WriteMaskFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::DepthFunc::Enum DepthFuncFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::TopoType::Enum TopoFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::TextureSlot::Enum TextureSlotFromStr(const char* pBegin, const char* pEnd);

    MATLIB_EXPORT render::StencilOperation::Enum StencilOpFromStr(const char* pBegin, const char* pEnd);
    MATLIB_EXPORT render::StencilFunc::Enum StencilFuncFromStr(const char* pBegin, const char* pEnd);

    // engine::AUTO_TILING for auto.
    MATLIB_EXPORT int16_t TilingSizeFromStr(const char* str);

    // helpers to get str len.
    X_INLINE MaterialMountType::Enum MatMountTypeFromStr(const char* str);
    X_INLINE MaterialCat::Enum MatCatFromStr(const char* str);
    X_INLINE MaterialUsage::Enum MatUsageFromStr(const char* str);
    X_INLINE MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str);
    X_INLINE MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str);
    X_INLINE render::FilterType::Enum FilterTypeFromStr(const char* str);
    X_INLINE render::TexRepeat::Enum TexRepeatFromStr(const char* str);
    X_INLINE render::CullType::Enum CullTypeFromStr(const char* str);
    X_INLINE render::BlendType::Enum BlendTypeFromStr(const char* str);
    X_INLINE render::BlendOp::Enum BlendOpFromStr(const char* str);

    X_INLINE render::StencilOperation::Enum StencilOpFromStr(const char* str);
    X_INLINE render::StencilFunc::Enum StencilFuncFromStr(const char* str);

} // namespace Util

X_NAMESPACE_END

#include "MatUtil.inl"