#pragma once

#include <ITexture.h>
#include "TextureFile.h"
#include "Util\Types.h"

namespace ispc
{
    struct rgba_surface;
} // namespace ispc

X_NAMESPACE_DECLARE(core,
                    class LinearAllocator)

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
    class ImgConveter
    {
    public:
        typedef core::traits::Function<void(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut)> CompressionFunc;

    private:
        struct MipFaceJobData
        {
            core::MemoryArenaBase* swapArena;

            XTextureFile& srcImg;

            const MipMapFilterParams& params;
            //  makes these enums 8bit if need smaller struct.
            MipFilter::Enum filter;
            WrapMode::Enum wrap;
            bool alpha;
            bool& result;

            int32_t faceIdx;
        };

    public:
        X_DECLARE_ENUM(Profile)
        (
            UltraFast,
            VeryFast,
            Fast,
            Basic,
            Slow
        );

    public:
        IMGLIB_EXPORT ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena);
        IMGLIB_EXPORT ~ImgConveter();
        
        IMGLIB_EXPORT void scale(ScaleFactor::Enum scale);
        IMGLIB_EXPORT bool resize(Vec2<uint16_t> dim, MipFilter::Enum mipFilter, WrapMode::Enum wrapMode);
        IMGLIB_EXPORT void enableMultiThreaded(bool enable);
        
        IMGLIB_EXPORT bool loadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt);
        IMGLIB_EXPORT bool loadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt);
        IMGLIB_EXPORT bool saveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);
        IMGLIB_EXPORT bool saveImg(core::XFile* pFile, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);
        
        IMGLIB_EXPORT bool addAlphachannel(bool keepMips = false);
        IMGLIB_EXPORT bool createMips(MipFilter::Enum filter, const MipMapFilterParams& params, WrapMode::Enum wrap, bool alpha, bool ignoreSrcMips = false);
        IMGLIB_EXPORT bool convert(Texturefmt::Enum targetFmt, Profile::Enum profile, bool keepAlpha = true);
        
        IMGLIB_EXPORT const XTextureFile& getTextFile(void) const;
        
        IMGLIB_EXPORT static void getDefaultFilterWidthAndParams(MipFilter::Enum filter, MipMapFilterParams& params);

    private:
        static void generateMipsForFace(MipFaceJobData& jobdata);
        static void generateMipsJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
        static void compressJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

        static CompressionFunc::Pointer getCompressionFunc(Texturefmt::Enum fmt, Profile::Enum profile, bool keepAlpha);

    private:
        core::MemoryArenaBase* swapArena_;
        XTextureFile srcImg_;
        XTextureFile dstImg_;

        // save out src.
        bool useSrc_;
        bool multiThread_;
    };

} // namespace Converter

X_NAMESPACE_END