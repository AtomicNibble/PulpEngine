#include "stdafx.h"
#include "ShaderPermatation.h"

#include <ICore.h>

#include "ShaderSourceTypes.h"
#include "HWShader.h"

X_NAMESPACE_BEGIN(render)

using namespace core;

namespace shader
{
    // flags for the diffrent input types that can be present in a shader.
    //
    //	base is awlways:
    //
    //		POS, UV
    //
    //		so we need omes for:
    //
    //		Normals
    //		BiNornmals & Tangents (always together)
    //		Color
    //
    //		Nolte we have diffrent types for some.
    //		like there is UV that is 3 floats.
    //		other uv's are 2 16bit floats.
    //
    //		but this is worked out in reflection.
    //		since i can work out what type they are.
    //
    //		this bit needs to be done pre compiel tho since i need to know what inputs type it even supports.
    //

    // ----------------------------------------------------

#if 1

    ShaderPermatation::ShaderPermatation(const ShaderStagesArr& stages, core::MemoryArenaBase* arena) :
        IlFmt_(InputLayoutFormat::Invalid),
        cbLinks_(arena)
    {
        X_ASSERT(stages_.size() == stages_.size(), "Stage aray sizes don't match")
        ();

        for (size_t i = 0; i < stages.size(); i++) {
            stages_[i] = static_cast<XHWShader*>(stages[i]);
        }
    }

    void ShaderPermatation::generateMeta(void)
    {
        if (isStageSet(ShaderType::Vertex)) {
            IlFmt_ = getStage(ShaderType::Vertex)->getILFormat();
        }
        else {
            X_ASSERT(IlFmt_ == InputLayoutFormat::Invalid, "Il should be invalid")
            ();
        }

        createCBLinks();
    }

    bool ShaderPermatation::isCompiled(void) const
    {
        for (const auto* pShader : stages_) {
            if (pShader && pShader->getStatus() != ShaderStatus::Ready) {
                return false;
            }
        }

        return true;
    }

    const ShaderPermatation::BufferArr& ShaderPermatation::getBuffers(void) const
    {
        // need to probs work something out for collecting buffers across stages.
        if (!isStageSet(ShaderType::Vertex)) {
            X_ASSERT_UNREACHABLE();
        }

        return getStage(ShaderType::Vertex)->getBuffers();
    }

    const ShaderPermatation::SamplerArr& ShaderPermatation::getSamplers(void) const
    {
        if (!isStageSet(ShaderType::Pixel)) {
            X_ASSERT_UNREACHABLE();
        }

        return getStage(ShaderType::Pixel)->getSamplers();
    }

    const ShaderPermatation::TexutreArr& ShaderPermatation::getTextures(void) const
    {
        if (!isStageSet(ShaderType::Pixel)) {
            X_ASSERT_UNREACHABLE();
        }

        return getStage(ShaderType::Pixel)->getTextures();
    }

    void ShaderPermatation::createCBLinks(void)
    {
        cbLinks_.clear();

        int32_t totalCBs = 0;

        for (auto* pShader : stages_) {
            if (pShader) {
                X_ASSERT(pShader->getStatus() == ShaderStatus::Ready, "All shaders should be compiled when creating CB links")
                ();
                addCBufsToLink(pShader);

                totalCBs += pShader->getNumConstantBuffers();
            }
        }

        if (totalCBs > MAX_SHADER_CB_PER_PERM) {
            X_ERROR("ShaderPerm", "Exceeded max cb's per perm");
        }
    }

    void ShaderPermatation::addCBufsToLink(XHWShader* pShader)
    {
        auto& cbufs = pShader->getCBuffers();
        auto stage = staderTypeToStageFlag(pShader->getType());

        for (auto& cb : cbufs) {
            // we match by name and size currently.
            for (auto& link : cbLinks_) {
                if (link.pCBufer->isEqual(cb)) {
                    link.stages.Set(stage);
                    goto skip_outer_loop;
                }
            }

            cbLinks_.emplace_back(stage, &cb);

        skip_outer_loop:;
        }
    }

#else

    XShaderTechniqueHW::XShaderTechniqueHW(core::MemoryArenaBase* arena) :
        cbLinks(arena)
    {
        cbLinks.setGranularity(2);

        IlFmt = InputLayoutFormat::Invalid;
        pVertexShader = nullptr;
        pPixelShader = nullptr;
        pGeoShader = nullptr;
        pHullShader = nullptr;
        pDomainShader = nullptr;
    }

    bool XShaderTechniqueHW::canDraw(void) const
    {
        bool canDraw = true;

        if (pVertexShader) {
            canDraw &= pVertexShader->getStatus() == ShaderStatus::Ready;
        }
        if (pPixelShader) {
            canDraw &= pPixelShader->getStatus() == ShaderStatus::Ready;
        }
        if (pGeoShader) {
            canDraw &= pGeoShader->getStatus() == ShaderStatus::Ready;
        }
        if (pHullShader) {
            canDraw &= pHullShader->getStatus() == ShaderStatus::Ready;
        }
        if (pDomainShader) {
            canDraw &= pDomainShader->getStatus() == ShaderStatus::Ready;
        }

        return canDraw;
    }

    bool XShaderTechniqueHW::tryCompile(bool forceSync)
    {
        if (pVertexShader && pVertexShader->getStatus() == ShaderStatus::NotCompiled) {
            if (!pVertexShader->compile(forceSync)) {
                return false;
            }

            addCbufs(pVertexShader);

            IlFmt = pVertexShader->getILFormat();
        }
        if (pPixelShader && pPixelShader->getStatus() == ShaderStatus::NotCompiled) {
            if (!pPixelShader->compile(forceSync)) {
                return false;
            }

            addCbufs(pPixelShader);
        }

        return true;
    }

    const XShaderTechniqueHW::CBufLinksArr& XShaderTechniqueHW::getCbufferLinks(void) const
    {
        return cbLinks;
    }

    void XShaderTechniqueHW::addCbufs(XHWShader* pShader)
    {
        auto& cbufs = pShader->getCBuffers();
        for (auto& cb : cbufs) {
            // we match by name and size currently.
            for (auto& link : cbLinks) {
                if (link.pCBufer->isEqual(cb)) {
                    //	link.stages.Set(pShader->getType());
                    goto skip_outer_loop;
                }
            }

            cbLinks.emplace_back(pShader->getType(), &cb);

        skip_outer_loop:;
        }
    }

#endif

} // namespace shader

X_NAMESPACE_END
