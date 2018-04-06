#pragma once

#include <ITexture.h>

X_NAMESPACE_BEGIN(texture)

class ImgLib : public IImgLib
{
public:
    ImgLib();
    ~ImgLib() X_OVERRIDE;

    virtual const char* getOutExtension(void) const X_OVERRIDE;
    virtual bool thumbGenerationSupported(void) const X_OVERRIDE;

    virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_OVERRIDE;
    virtual bool CreateThumb(IConverterHost& host, int32_t assetId, Vec2i targetDim) X_OVERRIDE;

private:
};

X_NAMESPACE_END