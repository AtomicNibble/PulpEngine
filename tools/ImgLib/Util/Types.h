#pragma once

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
    X_DECLARE_FLAGS(CompileFlag)
    (
        NOMIPS,
        IGNORE_SRC_MIPS,
        PREMULTIPLY_ALPHA,
        ALLOW_NONE_POW2,

        ALPHA,

        STREAMABLE,       // can be streamed
        HI_MIP_STREAMING, // only high mips can be streamed.
        FORCE_STREAM,     // force stream even if only one mip

        NO_COMPRESSION // leave as raw.
    );

    // we support producing a base mip smaller than original.
    // so you can provide a 4096x4096 source apply 1/4 scaling and get a 1024x1024 base mip.
    X_DECLARE_ENUM(ScaleFactor)
    (
        ORIGINAL,
        HALF,
        FOURTH,
        EIGHTH);

    X_DECLARE_ENUM(MipFilter)
    (
        Box,
        Triangle,
        Kaiser);

    X_DECLARE_ENUM(WrapMode)
    (
        Mirror,
        Repeat,
        Clamp);

    typedef Flags<CompileFlag> CompileFlags;

    struct MipMapFilterParams
    {
        float filterWidth;
        float params[2];
    };

} // namespace Converter

X_NAMESPACE_END