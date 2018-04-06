#include "stdafx.h"
#include "StateHelpers.h"

#include "SamplerDesc.h"

X_NAMESPACE_BEGIN(render)

namespace
{
    static const uint8 g_StencilFuncLookup[9] = {
        D3D12_COMPARISON_FUNC_NEVER, // pad
        D3D12_COMPARISON_FUNC_NEVER,
        D3D12_COMPARISON_FUNC_LESS,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_COMPARISON_FUNC_GREATER,
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        D3D12_COMPARISON_FUNC_EQUAL,
        D3D12_COMPARISON_FUNC_NOT_EQUAL,
        D3D12_COMPARISON_FUNC_ALWAYS,
    };

    static const uint8 g_StencilOpLookup[9] = {
        D3D12_STENCIL_OP_KEEP, // pad
        D3D12_STENCIL_OP_KEEP,
        D3D12_STENCIL_OP_ZERO,
        D3D12_STENCIL_OP_REPLACE,
        D3D12_STENCIL_OP_INCR_SAT,
        D3D12_STENCIL_OP_DECR_SAT,
        D3D12_STENCIL_OP_INVERT,
        D3D12_STENCIL_OP_INCR,
        D3D12_STENCIL_OP_DECR,
    };

} // namespace

void createDescFromState(const StateDesc& state, D3D12_BLEND_DESC& blendDesc)
{
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.AlphaToCoverageEnable = state.stateFlags.IsSet(StateFlag::ALPHATEST);

    if (state.stateFlags.IsSet(StateFlag::BLEND)) {
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
        blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;

        const auto blendState = state.blend;

        // A combination of D3D12_COLOR_WRITE_ENABLE-typed values that are combined by using a bitwise OR operation.
        blendDesc.RenderTarget[0].RenderTargetWriteMask = 0;

        const WriteMaskFlags fullMask(WriteMask::RED | WriteMask::GREEN | WriteMask::BLUE | WriteMask::ALPHA);

        if (blendState.writeMask.ToInt() == fullMask.ToInt()) {
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }
        else {
            if (blendState.writeMask.IsSet(WriteMask::RED)) {
                blendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
            }
            if (blendState.writeMask.IsSet(WriteMask::GREEN)) {
                blendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
            }
            if (blendState.writeMask.IsSet(WriteMask::BLUE)) {
                blendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
            }
            if (blendState.writeMask.IsSet(WriteMask::ALPHA)) {
                blendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
            }
        }

        auto blendTypeResolve = [](BlendType::Enum type) -> auto
        {
            switch (type) {
                case BlendType::ZERO:
                    return D3D12_BLEND_ZERO;
                case BlendType::ONE:
                    return D3D12_BLEND_ONE;

                case BlendType::SRC_COLOR:
                    return D3D12_BLEND_SRC_COLOR;
                case BlendType::SRC_ALPHA:
                    return D3D12_BLEND_SRC_ALPHA;
                case BlendType::SRC_ALPHA_SAT:
                    return D3D12_BLEND_SRC_ALPHA_SAT;
                case BlendType::SRC1_COLOR:
                    return D3D12_BLEND_SRC1_COLOR;
                case BlendType::SRC1_ALPHA:
                    return D3D12_BLEND_SRC1_ALPHA;

                case BlendType::INV_SRC_COLOR:
                    return D3D12_BLEND_INV_SRC_COLOR;
                case BlendType::INV_SRC_ALPHA:
                    return D3D12_BLEND_INV_SRC_ALPHA;

                case BlendType::INV_SRC1_COLOR:
                    return D3D12_BLEND_INV_SRC1_COLOR;
                case BlendType::INV_SRC1_ALPHA:
                    return D3D12_BLEND_INV_SRC1_ALPHA;

                case BlendType::DEST_COLOR:
                    return D3D12_BLEND_DEST_COLOR;
                case BlendType::DEST_ALPHA:
                    return D3D12_BLEND_DEST_ALPHA;

                case BlendType::INV_DEST_COLOR:
                    return D3D12_BLEND_INV_DEST_COLOR;
                case BlendType::INV_DEST_ALPHA:
                    return D3D12_BLEND_INV_DEST_ALPHA;

                case BlendType::BLEND_FACTOR:
                    return D3D12_BLEND_BLEND_FACTOR;
                case BlendType::INV_BLEND_FACTOR:
                    return D3D12_BLEND_INV_BLEND_FACTOR;

                case BlendType::INVALID:
                    X_ERROR("Dx12", "Invalid blend type passed to render device");
                    return D3D12_BLEND_ZERO;

                default:
#if X_DEBUG
                    X_ASSERT_NOT_IMPLEMENTED();
                    return D3D12_BLEND_ZERO;
#else
                    X_NO_SWITCH_DEFAULT;
#endif // X_DEBUG
            }
        };

        //Blending operation
        auto bledOpResolve = [](BlendOp::Enum op) -> auto
        {
            D3D12_BLEND_OP blendOperation = D3D12_BLEND_OP_ADD;
            switch (op) {
                case BlendOp::OP_ADD:
                    blendOperation = D3D12_BLEND_OP_ADD;
                    break;
                case BlendOp::OP_SUB:
                    blendOperation = D3D12_BLEND_OP_SUBTRACT;
                    break;
                case BlendOp::OP_REB_SUB:
                    blendOperation = D3D12_BLEND_OP_REV_SUBTRACT;
                    break;
                case BlendOp::OP_MIN:
                    blendOperation = D3D12_BLEND_OP_MIN;
                    break;
                case BlendOp::OP_MAX:
                    blendOperation = D3D12_BLEND_OP_MAX;
                    break;

                default:
                    X_NO_SWITCH_DEFAULT_ASSERT;
            }

            return blendOperation;
        };

        blendDesc.RenderTarget[0].SrcBlend = blendTypeResolve(blendState.srcBlendColor);
        blendDesc.RenderTarget[0].SrcBlendAlpha = blendTypeResolve(blendState.srcBlendAlpha);
        blendDesc.RenderTarget[0].DestBlend = blendTypeResolve(blendState.dstBlendColor);
        blendDesc.RenderTarget[0].DestBlendAlpha = blendTypeResolve(blendState.dstBlendAlpha);

        // todo: add separate alpha blend support for mrt
        blendDesc.RenderTarget[0].BlendOp = bledOpResolve(blendState.colorOp);
        blendDesc.RenderTarget[0].BlendOpAlpha = bledOpResolve(blendState.alphaOp);

        for (size_t i = 1; i < 8; ++i) {
            std::memcpy(&blendDesc.RenderTarget[i], &blendDesc.RenderTarget[i - 1], sizeof(blendDesc.RenderTarget[0]));
        }
    }
    else {
        // disabling 'BlendEnable' is not actually enougth... it still requires valid values.
        D3D12_RENDER_TARGET_BLEND_DESC defaultDesc = {
            FALSE,
            FALSE,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL};

        for (size_t i = 0; i < 8; ++i) {
            std::memcpy(&blendDesc.RenderTarget[i], &defaultDesc, sizeof(defaultDesc));
        }
    }
}

void createDescFromState(const StateDesc& state, D3D12_RASTERIZER_DESC& rasterizerDesc)
{
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = TRUE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    if (state.stateFlags.IsSet(StateFlag::WIREFRAME)) {
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    }

    switch (state.cullType) {
        case CullType::NONE:
            rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
            break;
        case CullType::FRONT_SIDED:
            rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
            break;
        case CullType::BACK_SIDED:
            rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
            break;

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }
}

void createDescFromState(const StateDesc& stateDesc, D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.StencilEnable = FALSE;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

    // Stencil operations if pixel is front-facing.
    {
        auto& frontFace = depthStencilDesc.FrontFace;
        const auto& state = stateDesc.stencil.front;

        frontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.zFailOp]);
        frontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.failOp]);
        frontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.passOp]);
        frontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(g_StencilFuncLookup[state.stencilFunc]);
    }

    // Stencil operations if pixel is back-facing.
    {
        auto& backFace = depthStencilDesc.BackFace;
        const auto& state = stateDesc.stencil.back;

        backFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.zFailOp]);
        backFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.failOp]);
        backFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.passOp]);
        backFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(g_StencilFuncLookup[state.stencilFunc]);
    }

    if (stateDesc.stateFlags.IsSet(StateFlag::DEPTHWRITE)) {
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    }
    if (stateDesc.stateFlags.IsSet(StateFlag::NO_DEPTH_TEST)) {
        depthStencilDesc.DepthEnable = FALSE;
    }
    if (stateDesc.stateFlags.IsSet(StateFlag::STENCIL)) {
        depthStencilDesc.StencilEnable = TRUE;
    }

    switch (stateDesc.depthFunc) {
        case DepthFunc::LEQUAL:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
            break;
        case DepthFunc::EQUAL:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
            break;
        case DepthFunc::GREAT:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
            break;
        case DepthFunc::LESS:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            break;
        case DepthFunc::GEQUAL:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            break;
        case DepthFunc::NOTEQUAL:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
            break;
        case DepthFunc::ALWAYS:
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            break;

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    if (!DEPTH_REVERSE_Z) {
        // all the states are setup for reverse Z.
        // if we want to disable it for testing, we flip the states.
        switch (stateDesc.depthFunc) {
            case DepthFunc::LEQUAL:
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
                break;
            case DepthFunc::EQUAL:
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
                break;
            case DepthFunc::GREAT:
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
                break;
            case DepthFunc::LESS:
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
                break;
            case DepthFunc::GEQUAL:
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
                break;
            default:
                break;
        }
    }
}

void samplerDescFromState(SamplerState state, SamplerDesc& desc)
{
    switch (state.repeat) {
        case TexRepeat::NO_TILE:
            desc.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
            break;

        case TexRepeat::TILE_BOTH:
            desc.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_MIRROR);
            break;
        case TexRepeat::TILE_HOZ:
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case TexRepeat::TILE_VERT:
            desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
    }

    switch (state.filter) {
        case FilterType::NEAREST_MIP_NONE:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            desc.MaxLOD = 0;
            break;
        case FilterType::NEAREST_MIP_NEAREST:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            break;
        case FilterType::NEAREST_MIP_LINEAR:
            desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            break;

        case FilterType::LINEAR_MIP_NONE:
            desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            desc.MaxLOD = 0;
            break;
        case FilterType::LINEAR_MIP_NEAREST:
            desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
        case FilterType::LINEAR_MIP_LINEAR:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;

        case FilterType::ANISOTROPIC_X2:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;
            desc.MaxAnisotropy = 2;
            break;
        case FilterType::ANISOTROPIC_X4:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;
            desc.MaxAnisotropy = 4;
            break;
        case FilterType::ANISOTROPIC_X8:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;
            desc.MaxAnisotropy = 8;
            break;
        case FilterType::ANISOTROPIC_X16:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;
            desc.MaxAnisotropy = 16;
            break;
    }
}

X_NAMESPACE_END