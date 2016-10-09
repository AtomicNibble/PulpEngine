#pragma once


#include <ITexture.h>
#include "TextureFile.h"
#include "Types.h"

X_NAMESPACE_DECLARE(core,
	class LinearAllocator;
)

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
	
	class ImgConveter
	{
		static const size_t MAX_FMT_CLASS_SIZE;
		typedef core::traits::Function<void(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut)> CompressionFunc;

		struct JobData
		{
			ImgConveter::CompressionFunc::Pointer pCompressFunc;

			ispc::rgba_surface surface;
			uint8_t* pOut;
		};

		X_DECLARE_ENUM(Profile)(
			UltraFast,
			VeryFast,
			Fast,
			Basic,
			Slow
		);

	public:
		ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena);
		~ImgConveter();

		void enableMultiThreaded(bool enable);

		bool LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt);
		bool LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt);
		bool SaveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);

		bool addAlphachannel(bool keepMips = false);
		bool CreateMips(MipFilter::Enum filter, const MipMapFilterParams& params, WrapMode::Enum wrap, bool alpha, bool ignoreSrcMips = false);
		bool Convert(Texturefmt::Enum targetFmt, bool keepAlpha = true);

		const XTextureFile& getTextFile(void) const;

		static bool ParseImgFmt(const char* pImgFmt, Texturefmt::Enum& fmtOut);
		static void getDefaultFilterWidthAndParams(MipFilter::Enum filter, MipMapFilterParams& params);

	private:
		static void compressJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

		static CompressionFunc::Pointer getCompressionFunc(Texturefmt::Enum fmt, Profile::Enum profile, bool keepAlpha);
		static ITextureFmt* Allocfmt(core::LinearAllocator* pAllocator, ImgFileFormat::Enum inputFileFmt);

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