#pragma once


#include <ITexture.h>
#include "TextureFile.h"

X_NAMESPACE_DECLARE(core,
	class LinearAllocator;
)


X_NAMESPACE_BEGIN(texture)

namespace Converter
{
	X_DECLARE_FLAGS(CompileFlag)(
		NO_MIPS,
		PREMULTIPLY_ALPHA,
		ALLOW_NONE_POW2,

		STREAMABLE,			// can be streamed
		HI_MIP_STREAMING,	// only high mips can be streamed.
		FORCE_STREAM,		// force stream even if only one mip

		NO_COMPRESSION		// leave as raw.
	);


	// we support producing a base mip smaller than original.
	// so you can provide a 4096x4096 source apply 1/4 scaling and get a 1024x1024 base mip.
	X_DECLARE_ENUM(ScaleFactor) (
		ORIGINAL,
		HALF,
		QUARTER,
		FOURTH,
		EIGHTH
	);

	X_DECLARE_ENUM(ImgFileFormat) (
		CI,
		DDS,
		PNG,
		JPG,
		PSD,
		TGA,

		UNKNOWN
	);

	X_DECLARE_ENUM(MipFilter) (
		DEFAULT
	);

	X_DECLARE_ENUM(WrapMode) (
		Mirror,
		Repeat,
		Clamp
	);

	typedef Flags<CompileFlag> CompileFlags;


	class ImgConveter
	{
		static const size_t MAX_FMT_CLASS_SIZE;

	public:
		ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena);
		~ImgConveter();


		bool LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt);
		bool LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt);
		bool SaveImg(const core::Path<char>& outPath, CompileFlag flags, ImgFileFormat::Enum dstFileFmt);

		bool CreateMips(MipFilter::Enum filter, WrapMode::Enum wrap);
		bool Convert(Texturefmt::Enum targetFmt);

		static bool ParseImgFmt(const char* pImgFmt, Texturefmt::Enum& fmtOut);
		static ITextureFmt* Allocfmt(core::LinearAllocator* pAllocator, ImgFileFormat::Enum inputFileFmt);

	private:

	private:
		core::MemoryArenaBase* swapArena_;
		XTextureFile srcImg_;

	};





} // namespace Converter


X_NAMESPACE_END