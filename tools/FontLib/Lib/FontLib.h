#pragma once

X_NAMESPACE_BEGIN(font)

class FontLib : public IFontLib
{
public:
    FontLib();
    ~FontLib() X_OVERRIDE;

    virtual const char* getOutExtension(void) const X_OVERRIDE;

    virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;
};

X_NAMESPACE_END