#pragma once

X_NAMESPACE_BEGIN(render)

struct ShaderModel
{
    ShaderModel() :
        majorVer(0),
        minorVer(0)
    {
    }
    ShaderModel(uint8_t major, uint8_t minor) :
        majorVer(major),
        minorVer(minor)
    {
    }

    uint8_t majorVer : 6;
    uint8_t minorVer : 2;
};

struct GpuFeatures
{
    GpuFeatures()
    {
        core::zero_this(this);
    }

    ShaderModel maxShaderModel;

    uint32_t maxTextureWidth;
    uint32_t maxTextureHeight;
    uint32_t maxTextureDepth;
    uint32_t maxTextureCubeSize;
    uint32_t maxTextureArrayLength;
    uint8_t maxVertexTextureUnits;
    uint8_t maxPixelTextureUnits;
    uint8_t maxGeometryTextureUnits;
    uint8_t maxSimultaneousRts;
    uint8_t maxSimultaneousUavs;
    uint8_t maxVertexStreams;
    uint8_t maxTextureAnisotropy;

    bool isTbdr : 1;             // TileBasedRenderer
    bool isUMA : 1;              //  unified memory architecture.
    bool isUMACacheCoherent : 1; //  unified memory architecture.

    bool hwInstancingSupport : 1;
    bool instanceIdSupport : 1;
    bool streamOutputSupport : 1;
    bool alphaToCoverageSupport : 1;
    bool primitiveRestartSupport : 1;
    bool multithreadRenderingSupport : 1;
    bool multithreadResCreatingSupport : 1;
    bool mrtIndependentBitDepthsSupport : 1;
    bool standardDerivativesSupport : 1;
    bool shaderTextureLodSupport : 1;
    bool logicOpSupport : 1;
    bool independentBlendSupport : 1;
    bool depthTextureSupport : 1;
    bool fpColorSupport : 1;
    bool packToRgbaRequired : 1;
    bool drawIndirectSupport : 1;
    bool noOverwriteSupport : 1;
    bool fullNpotTextureSupport : 1;
    bool renderToTextureArraySupport : 1;

    bool gsSupport : 1;
    bool csSupport : 1;
    bool hsSupport : 1;
    bool dsSupport : 1;

    bool init : 1;
};

X_NAMESPACE_END
