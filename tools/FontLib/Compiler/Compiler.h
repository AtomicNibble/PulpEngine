#pragma once

#include <Containers\ByteStream.h>

#include "FontRender\XFontRender.h"

X_NAMESPACE_DECLARE(core,
                    struct XFile);

X_NAMESPACE_BEGIN(font)


struct FontEffect
{
    FontEffect(core::MemoryArenaBase* arena) :
        passes(arena)
    {
    }

    core::StackString<64> name;
    core::Array<FontPass> passes;
};


class FontCompiler
{
    typedef core::Array<uint8_t> DataVec;
    typedef core::Array<XGlyph> XGlyphArr;
    typedef core::Array<FontEffect> EffetsArr;

public:
    FontCompiler(core::MemoryArenaBase* arena);
    ~FontCompiler();

    bool setFont(DataVec&& trueTypeData, int32_t width, int32_t height, float sizeRatio);
    bool bake(bool sdf);
    bool loadFromJson(core::string& str);

    bool writeToFile(core::XFile* pFile) const;
    bool writeImageToFile(core::XFile* pFile) const;

private:
    core::MemoryArenaBase* arena_;
    FontFlags flags_;
    XGlyphArr glyphs_;
    EffetsArr effects_;

    DataVec sourceFontData_;

    XFontRender render_;
};

X_NAMESPACE_END