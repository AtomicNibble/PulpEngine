#pragma once

#include <ITexture.h>

X_NAMESPACE_BEGIN(texture)

class ImgLib : public IImgLib
{
public:
    ImgLib();
    ~ImgLib() X_FINAL;

    const char* getOutExtension(void) const X_FINAL;
    bool thumbGenerationSupported(void) const X_FINAL;
    bool repackSupported(void) const X_FINAL;

    bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_FINAL;
    bool CreateThumb(IConverterHost& host, int32_t assetId, Vec2i targetDim) X_FINAL;
    bool Repack(IConverterHost& host, assetDb::AssetId assetId) const X_FINAL;
    
private:
};

X_NAMESPACE_END