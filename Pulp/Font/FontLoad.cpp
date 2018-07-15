#include "stdafx.h"
#include "Font.h"

#include "Sys\XFontSystem.h"
#include "FontRender\XFontTexture.h"

#include <IFileSys.h>
#include <Threading\JobSystem2.h>

#include <Memory\MemCursor.h>

X_NAMESPACE_BEGIN(font)

using namespace core;


bool XFont::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
{
    if (dataSize < sizeof(FontHdr)) {
        return false;
    }

    FontHdr& hdr = *reinterpret_cast<FontHdr*>(data.get());

    if (!hdr.isValid()) {
        X_ERROR("Font", "\"%s\" header is invalid", name_.c_str());
        return false;
    }

    if (hdr.numEffects < 1) {
        X_ERROR("Font", "\"%s\" has no effects", name_.c_str());
        return false;
    }

    core::MemCursor cursor(data.ptr() + sizeof(hdr), dataSize - sizeof(hdr));

    // each effect has multiple passes.
    effectsHdr_ = core::make_span(cursor.postSeekPtr<FontEffectHdr>(hdr.numEffects), hdr.numEffects);
    effectsPasses_ = core::make_span(cursor.postSeekPtr<FontPass>(hdr.numPasses), hdr.numPasses);

    // baked glphys hdr
    bakedGlyphs_ = core::make_span(cursor.postSeekPtr<GlyphHdr>(hdr.numGlyphs), hdr.numGlyphs);
    
    // baked glphys bitmaps
    auto bakedDataSize = (hdr.glyphWidth * hdr.glyphHeight) * hdr.numGlyphs;
    bakedData_ = core::make_span(cursor.postSeekPtr<char>(bakedDataSize), bakedDataSize);

    // src.
    X_ASSERT(hdr.sourceFontSize == cursor.numBytesRemaning(), "Font parse error")(hdr.sourceFontSize, cursor.numBytesRemaning());

    fontSrc_ = core::make_span(cursor.postSeekPtr<uint8_t>(hdr.sourceFontSize), hdr.sourceFontSize);

    // Make a font texture for it.
    pFontTexture_ = core::makeUnique<XFontTexture>(g_fontArena, fontSys_.getVars(), g_fontArena);

    // setup the font texture cpu buffers.
    if (!pFontTexture_->Create(512, 512, 16, 16, bakedGlyphs_, bakedData_, fontSrc_)) {
        X_ERROR("Font", "\"%s\" failed to create font texture", name_.c_str());
        return false;
    }

    if (fontSys_.getVars().glyphCachePreWarm()) {
        pFontTexture_->PreWarmCache();
    }

    data_ = std::move(data);
    return true;
}

#if 0
bool XFont::processData(const char* pBegin, const char* pEnd, SourceNameStr& sourceNameOut,
    EffetsArr& effectsOut, FontFlags& flags)
{
    ptrdiff_t size = (pEnd - pBegin);
    if (!size) {
        return false;
    }

    effectsOut.clear();
    sourceNameOut.clear();
    flags.Clear();

    core::json::MemoryStream ms(pBegin, size);
    core::json::EncodedInputStream<core::json::UTF8<>, core::json::MemoryStream> is(ms);

    core::json::Document d;
    if (d.ParseStream<core::json::kParseCommentsFlag>(is).HasParseError()) {
        auto err = d.GetParseError();
        const char* pErrStr = core::json::GetParseError_En(err);
        size_t offset = d.GetErrorOffset();
        size_t line = core::strUtil::LineNumberForOffset(pBegin, pEnd, offset);

        X_ERROR("Font", "Failed to parse font desc(%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s",
            err, offset, line, pErrStr);
        return false;
    }

    if (d.GetType() != core::json::Type::kObjectType) {
        X_ERROR("Font", "Unexpected type");
        return false;
    }

    if (!d.HasMember("font")) {
        X_ERROR("Font", "Missing font object");
        return false;
    }

    auto& font = d["font"];
    if (!font.HasMember("source")) {
        X_ERROR("Font", "Missing source field");
        return false;
    }

    auto& source = font["source"];
    sourceNameOut.set(source.GetString(), source.GetString() + source.GetStringLength());

    if (font.HasMember("proportional")) {
        if (font["proportional"].GetBool()) {
            flags.Set(FontFlag::PROPORTIONAL);
        }
    }
    if (font.HasMember("sdf")) {
        if (font["sdf"].GetBool()) {
            flags.Set(FontFlag::SDF);
        }
    }

    if (font.HasMember("effects"))
    {
        auto& effects = font["effects"];
        if (effects.GetType() != core::json::Type::kArrayType) {
            X_ERROR("Font", "Effects field should be a array");
            return false;
        }

        for (auto& effectDesc : effects.GetArray())
        {
            FontEffect effect;

            if (effectDesc.HasMember("name")) {
                auto& name = effectDesc["name"];
                effect.name.set(name.GetString(), name.GetString() + name.GetStringLength());
            }

            if (!effectDesc.HasMember("passes")) {
                X_ERROR("Font", "Missing passes field");
                return false;
            }
            
            auto& passes = effectDesc["passes"];
            if (passes.GetType() != core::json::Type::kArrayType) {
                X_ERROR("Font", "Passes field should be a array");
                return false;
            }

            for (auto& passDesc : passes.GetArray())
            {
                FontPass pass;

                if (passDesc.HasMember("color")) {
                    X_ASSERT_NOT_IMPLEMENTED();
                }
                if (passDesc.HasMember("pos")) {
                    X_ASSERT_NOT_IMPLEMENTED();
                }

                effect.passes.append(pass);
            }

            if (effect.passes.isEmpty())
            {
                X_ERROR("Font", "Effect has no passes");
                return false;
            }

            effectsOut.append(effect);
        }
    }

    if (effectsOut.isEmpty())
    {
        X_WARNING("Font", "No effects provided, adding default");

        FontEffect effect;
        effect.passes.resize(1);
        effectsOut.append(effect);
    }

    return true;
}
#endif

X_NAMESPACE_END
