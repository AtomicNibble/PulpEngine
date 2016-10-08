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

	public:
		ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena);
		~ImgConveter();


		bool LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt);
		bool LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt);
		bool SaveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt);

		bool addAlphachannel(bool keepMips = false);
		bool CreateMips(MipFilter::Enum filter, const MipMapFilterParams& params, WrapMode::Enum wrap, bool alpha, bool ignoreSrcMips = false);
		bool Convert(Texturefmt::Enum targetFmt);

		const XTextureFile& getTextFile(void) const;

		static bool ParseImgFmt(const char* pImgFmt, Texturefmt::Enum& fmtOut);
		static void getDefaultFilterWidthAndParams(MipFilter::Enum filter, MipMapFilterParams& params);

	private:
		static ITextureFmt* Allocfmt(core::LinearAllocator* pAllocator, ImgFileFormat::Enum inputFileFmt);

	private:
		core::MemoryArenaBase* swapArena_;
		XTextureFile srcImg_;
		XTextureFile dstImg_;

		// save out src.
		bool useSrc_;
	};





} // namespace Converter


X_NAMESPACE_END