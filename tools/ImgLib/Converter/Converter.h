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
            Slow);

    public:
        ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena);
        ~ImgConveter();

        void scale(ScaleFactor::Enum scale);
        void enableMultiThreaded(bool enable);

        bool LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt);
        bool LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt);
        bool SaveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);
        bool SaveImg(core::XFile* pFile, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);

        bool addAlphachannel(bool keepMips = false);
        bool CreateMips(MipFilter::Enum filter, const MipMapFilterParams& params, WrapMode::Enum wrap, bool alpha, bool ignoreSrcMips = false);
        bool Convert(Texturefmt::Enum targetFmt, Profile::Enum profile, bool keepAlpha = true);

        const XTextureFile& getTextFile(void) const;

        static void getDefaultFilterWidthAndParams(MipFilter::Enum filter, MipMapFilterParams& params);

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