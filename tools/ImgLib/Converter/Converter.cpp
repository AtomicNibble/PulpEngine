#include "stdafx.h"
#include "Converter.h"


#include <String\StringHash.h>

#include "Fmts\TextureLoaderCI.h"
#include "Fmts\TextureLoaderDDS.h"
#include "Fmts\TextureLoaderJPG.h"
#include "Fmts\TextureLoaderPNG.h"
#include "Fmts\TextureLoaderPSD.h"
#include "Fmts\TextureLoaderTGA.h"

#include <Memory/AllocationPolicies/LinearAllocator.h>

#include "Filters.h"
#include "FloatImage.h"


X_NAMESPACE_BEGIN(texture)


namespace Converter
{


	namespace
	{
		struct FmtStr
		{
			Texturefmt::Enum fmt;
			core::StrHash nameHash;
			const char* pName;
		};

		FmtStr formats[] =
		{
			{ Texturefmt::A8, core::StrHash("A8"), "A8" },
			{ Texturefmt::R8G8, core::StrHash("R8G8"), "R8G8" },
			{ Texturefmt::R8G8_TYPELESS, core::StrHash("R8G8_TYPELESS"), "R8G8_TYPELESS" },
			{ Texturefmt::R8G8_SNORM, core::StrHash("R8G8_SNORM"), "R8G8_SNORM" },
			{ Texturefmt::R8G8_UNIT, core::StrHash("R8G8_UNIT"), "R8G8_UNIT" },
			{ Texturefmt::R8G8_SINT, core::StrHash("R8G8_SINT"), "R8G8_SINT" },
			{ Texturefmt::R16G16_FLOAT, core::StrHash("R16G16_FLOAT"), "R16G16_FLOAT" },
			{ Texturefmt::R16G16, core::StrHash("R16G16"), "R16G16" },
			{ Texturefmt::R16G16_SRGB, core::StrHash("R16G16_SRGB"), "R16G16_SRGB" },
			{ Texturefmt::R16G16_SNORM, core::StrHash("R16G16_SNORM"), "R16G16_SNORM" },
			{ Texturefmt::R16G16_SINT, core::StrHash("R16G16_SINT"), "R16G16_SINT" },
			{ Texturefmt::R16G16_UINT, core::StrHash("R16G16_UINT"), "R16G16_UINT" },
			{ Texturefmt::R16G16_TYPELESS, core::StrHash("R16G16_TYPELESS"), "R16G16_TYPELESS" },
			{ Texturefmt::R8G8B8, core::StrHash("R8G8B8"), "R8G8B8" },
			{ Texturefmt::B8G8R8, core::StrHash("B8G8R8"), "B8G8R8" },
			{ Texturefmt::R8G8B8A8, core::StrHash("R8G8B8A8"), "R8G8B8A8" },
			{ Texturefmt::R8G8B8A8_SRGB, core::StrHash("R8G8B8A8_SRGB"), "R8G8B8A8_SRGB" },
			{ Texturefmt::R8G8B8A8_SNORM, core::StrHash("R8G8B8A8_SNORM"), "R8G8B8A8_SNORM" },
			{ Texturefmt::R8G8B8A8_TYPELESS, core::StrHash("R8G8B8A8_TYPELESS"), "R8G8B8A8_TYPELESS" },
			{ Texturefmt::R8G8B8A8_TYPELESS, core::StrHash("R8G8B8A8_TYPELESS"), "R8G8B8A8_TYPELESS" },
			{ Texturefmt::R8G8B8A8_SINT, core::StrHash("R8G8B8A8_SINT"), "R8G8B8A8_SINT" },
			{ Texturefmt::R8G8B8A8_UINT, core::StrHash("R8G8B8A8_UINT"), "R8G8B8A8_UINT" },
			{ Texturefmt::A8R8G8B8, core::StrHash("A8R8G8B8"), "A8R8G8B8" },
			{ Texturefmt::B8G8R8A8, core::StrHash("B8G8R8A8"), "B8G8R8A8" },
			{ Texturefmt::B8G8R8A8_SRGB, core::StrHash("B8G8R8A8_SRGB"), "B8G8R8A8_SRGB" },
			{ Texturefmt::B8G8R8A8_TYPELESS, core::StrHash("B8G8R8A8_TYPELESS"), "B8G8R8A8_TYPELESS" },
			{ Texturefmt::ATI2, core::StrHash("ATI2"), "ATI2" },
			{ Texturefmt::ATI2_XY, core::StrHash("ATI2_XY"), "ATI2_XY" },
			{ Texturefmt::BC1, core::StrHash("BC1"), "BC1" },
			{ Texturefmt::BC1_SRGB, core::StrHash("BC1_SRGB"), "BC1_SRGB" },
			{ Texturefmt::BC1_TYPELESS, core::StrHash("BC1_TYPELESS"), "BC1_TYPELESS" },
			{ Texturefmt::BC2, core::StrHash("BC2"), "BC2" },
			{ Texturefmt::BC2_SRGB, core::StrHash("BC2_SRGB"), "BC2_SRGB" },
			{ Texturefmt::BC2_TYPELESS, core::StrHash("BC2_TYPELESS"), "BC2_TYPELESS" },
			{ Texturefmt::BC3, core::StrHash("BC3"), "BC3" },
			{ Texturefmt::BC3_SRGB, core::StrHash("BC3_SRGB"), "BC3_SRGB" },
			{ Texturefmt::BC3_TYPELESS, core::StrHash("BC3_TYPELESS"), "BC3_TYPELESS" },
			{ Texturefmt::BC4, core::StrHash("BC4"), "BC4" },
			{ Texturefmt::BC4_SNORM, core::StrHash("BC4_SNORM"), "BC4_SNORM" },
			{ Texturefmt::BC4_TYPELESS, core::StrHash("BC4_TYPELESS"), "BC4_TYPELESS" },
			{ Texturefmt::BC5, core::StrHash("BC5"), "BC5" },
			{ Texturefmt::BC5_SNORM, core::StrHash("BC5_SNORM"), "BC5_SNORM" },
			{ Texturefmt::BC5_TYPELESS, core::StrHash("BC5_TYPELESS"), "BC5_TYPELESS" },
			{ Texturefmt::BC6, core::StrHash("BC6"), "BC6" },
			{ Texturefmt::BC6_SF16, core::StrHash("BC6_SF16"), "BC6_SF16" },
			{ Texturefmt::BC6_TYPELESS, core::StrHash("BC6_TYPELESS"), "BC6_TYPELESS" },
			{ Texturefmt::BC7, core::StrHash("BC7"), "BC7" },
			{ Texturefmt::BC7_SRGB, core::StrHash("BC7_SRGB"), "BC7_SRGB" },
			{ Texturefmt::BC7_TYPELESS, core::StrHash("BC7_TYPELESS"), "BC7_TYPELESS" },
			{ Texturefmt::R10G10B10A2, core::StrHash("R10G10B10A2"), "R10G10B10A2" },
		};


	} // namespace


	const size_t ImgConveter::MAX_FMT_CLASS_SIZE = core::Max<size_t>(
		core::Max(
			core::Max(
				core::Max(
					core::Max(
						core::Max(
							sizeof(DDS::XTexLoaderDDS),
							sizeof(CI::XTexLoaderCI)),
						sizeof(JPG::XTexLoaderJPG)),
					sizeof(TGA::XTexLoaderTGA)),
				sizeof(PSD::XTexLoaderPSD)),
			sizeof(PNG::XTexLoaderPNG)),
		16) + 256;


	ImgConveter::ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena) :
		swapArena_(swapArena),
		srcImg_(imgArena),
		dstImg_(imgArena),
		useSrc_(false)
	{

	}

	ImgConveter::~ImgConveter()
	{

	}


	bool ImgConveter::LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt)
	{
		core::XFileBuf file(fileData.begin(), fileData.end());

		return LoadImg(&file, inputFileFmt);
	}

	bool ImgConveter::LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt)
	{
		X_ALIGNED_SYMBOL(char buf[MAX_FMT_CLASS_SIZE], 16);
		core::LinearAllocator allocator(buf, buf + sizeof(buf));

		useSrc_ = false;

		ITextureFmt* pFmt = Allocfmt(&allocator, inputFileFmt);

		bool res = pFmt->loadTexture(pFile, srcImg_, swapArena_);

		core::Mem::Destruct<ITextureFmt>(pFmt);
		return res;
	}

	bool ImgConveter::SaveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt)
	{
		core::XFileScoped file;
		core::fileModeFlags mode;
		mode.Set(core::fileMode::WRITE);
		mode.Set(core::fileMode::RECREATE);

		X_ALIGNED_SYMBOL(char buf[MAX_FMT_CLASS_SIZE], 16);
		core::LinearAllocator allocator(buf, buf + sizeof(buf));
		ITextureFmt* pFmt = Allocfmt(&allocator, dstFileFmt);

		if (!pFmt->canWrite()) {
			core::Mem::Destruct<ITextureFmt>(pFmt);
			X_ERROR("Img", "Writing to fmt: %s is not supported", ImgFileFormat::ToString(dstFileFmt));
			return false;
		}

		std::remove_const_t<std::remove_reference_t<decltype(outPath)>> path(outPath);
		path.setExtension(pFmt->getExtension());

		if (!file.openFile(path.c_str(), mode)) {
			core::Mem::Destruct<ITextureFmt>(pFmt);
			return false;
		}

		// add in flags.
		auto& img = useSrc_ ? srcImg_ : dstImg_;

		if (!img.isValid()) {
			X_ERROR("Img", "Failed to save img it's invalid");
			return false;
		}

		{
			auto imgFlags = img.getFlags();

			// check for flags that should not be set.
			CompileFlags::Description flagDsc;
			X_ASSERT(!imgFlags.IsSet(TexFlag::LOAD_FAILED), "Load faild flag set")(flags.ToString(flagDsc));

			// clear some. (temp)
			imgFlags.Set(TexFlag::CI_IMG);
			imgFlags.Set(TexFlag::ALPHA);
			imgFlags.Set(TexFlag::NORMAL);

			if (flags.IsSet(CompileFlag::STREAMABLE)) {
				imgFlags.Set(TexFlag::STREAMABLE);
			}
			if (flags.IsSet(CompileFlag::HI_MIP_STREAMING)) {
				X_ASSERT(imgFlags.IsSet(TexFlag::STREAMABLE), "HimipStreaming set without streamable flag")(flags.ToString(flagDsc));
				imgFlags.Set(TexFlag::HI_MIP_STREAMING);
			}
			if (flags.IsSet(CompileFlag::FORCE_STREAM)) {
				// should STREAMBLE be set for force steam... hummm
				imgFlags.Set(TexFlag::FORCE_STREAM);
			}
			if (flags.IsSet(CompileFlag::NOMIPS)) {
				imgFlags.Set(TexFlag::NOMIPS);
			}
			if (flags.IsSet(CompileFlag::ALPHA)) {
				imgFlags.Set(TexFlag::ALPHA);
			}

			img.setFlags(imgFlags);
		}


		bool res = pFmt->saveTexture(file.GetFile(), img);

		core::Mem::Destruct<ITextureFmt>(pFmt);
		return res;
	}


	bool ImgConveter::CreateMips(MipFilter::Enum filter, const MipMapFilterParams& params, 
		WrapMode::Enum wrap, bool alpha, bool ignoreSrcMips)
	{
		const uint32_t curMips = srcImg_.getNumMips();
		const uint32_t requiredMips = Util::maxMipsForSize(srcImg_.getWidth(), srcImg_.getHeight());

		// if we have mips and it's the correct count return ok.
		// later on we might want to discard them and generate with a diffrent filter.
		// for now i could not give a shit, since we can always reprocess every image in one click.
		if (curMips > 1 && curMips == requiredMips && !ignoreSrcMips) {
			return true;
		}

		if (curMips > 1 && !ignoreSrcMips) {
			X_WARNING("Img", "Image has incorrect mip count of: %" PRIu32, " should be: %" PRIu32 " regenerating",
				curMips, requiredMips);

			// check this logic.
			X_ASSERT_NOT_IMPLEMENTED();
		}

		// create mips.
		// we might be ignoring srcmips OR making new.
		if (srcImg_.getNumFaces() > 1) {
			X_ASSERT_NOT_IMPLEMENTED();
			return false;
		}

		srcImg_.allocMipBuffers();
		X_ASSERT(srcImg_.getNumMips() == requiredMips, "Mip count mismatch")(srcImg_.getNumMips(), requiredMips);

		{
			FloatImage fltImg(swapArena_);
			FloatImage halfImg(swapArena_);

			// create from first mip and level
			fltImg.initFrom(srcImg_, 0, 0);

			for(uint32_t mip = 1; mip < requiredMips; mip++)
			{
				if (alpha)
				{
					if (filter == MipFilter::Box)
					{
						BoxFilter filter(params.filterWidth);
						fltImg.downSample(halfImg, swapArena_, filter, wrap, 3);
					}
					else if (filter == MipFilter::Triangle)
					{
						TriangleFilter filter(params.filterWidth);
						fltImg.downSample(halfImg, swapArena_, filter, wrap, 3);
					}
					else if (filter == MipFilter::Kaiser)
					{
						KaiserFilter filter(params.filterWidth);
						filter.setParameters(params.params[0], params.params[1]);
						fltImg.downSample(halfImg, swapArena_, filter, wrap, 3);
					}
					else
					{
						X_ASSERT_UNREACHABLE();
					}
				}
				else
				{
					if (filter == MipFilter::Box)
					{
						if (math<float>::abs(params.filterWidth - 0.5f) < EPSILON && srcImg_.getNumFaces() == 1) {
							fltImg.fastDownSample(halfImg);
						}
						else {
							BoxFilter filter(params.filterWidth);
							fltImg.downSample(halfImg, swapArena_, filter, wrap);
						}
					}
					else if (filter == MipFilter::Triangle)
					{
						TriangleFilter filter(params.filterWidth);
						fltImg.downSample(halfImg, swapArena_, filter, wrap);
					}
					else if (filter == MipFilter::Kaiser)
					{
						KaiserFilter filter(params.filterWidth);
						filter.setParameters(params.params[0], params.params[1]);
						fltImg.downSample(halfImg, swapArena_, filter, wrap);
					}
					else
					{
						X_ASSERT_UNREACHABLE();
					}
				}


				const uint32_t numPixels = halfImg.width() * halfImg.height();
				const float* pRedChannel = halfImg.channel(0);
				const float* pGreenChannel = halfImg.channel(1);
				const float* pBlueChannel = halfImg.channel(2);
				const float* pAlphaChannel = halfImg.channel(3);

				{
					const size_t dstSize = srcImg_.getLevelSize(mip);
					uint8_t* pMipDst = srcImg_.getLevel(0, mip);

					if (srcImg_.getFormat() == Texturefmt::R16G16B16A16_FLOAT)
					{
						// are we on the same page? (mentally)
						const size_t expectedSize = (numPixels * 8);
						X_ASSERT(dstSize == expectedSize, "Size missmatch for expected vs provided size")(dstSize, expectedSize);
						

						for (uint32_t i = 0; i < numPixels; i++)
						{
							uint16_t* pPixel = reinterpret_cast<uint16_t*>(&pMipDst[i * 8]);

							pPixel[0] = XHalfCompressor::compress(math<float>::clamp(pRedChannel[i], 0.f, 1.f));
							pPixel[1] = XHalfCompressor::compress(math<float>::clamp(pGreenChannel[i], 0.f, 1.f));
							pPixel[2] = XHalfCompressor::compress(math<float>::clamp(pBlueChannel[i], 0.f, 1.f));
							pPixel[3] = XHalfCompressor::compress(math<float>::clamp(pAlphaChannel[i], 0.f, 1.f));
						}
					}
					else
					{
						const size_t expectedSize = (numPixels * 4);
						X_ASSERT(dstSize == expectedSize, "Size missmatch for expected vs provided size")(dstSize, expectedSize);

						for (uint32_t i = 0; i < numPixels; i++)
						{
							uint8_t* pPixel = &pMipDst[i * 4];
							pPixel[0] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pRedChannel[i], 0.f, 1.f));
							pPixel[1] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pGreenChannel[i], 0.f, 1.f));
							pPixel[2] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pBlueChannel[i], 0.f, 1.f));
							pPixel[3] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pAlphaChannel[i], 0.f, 1.f));
						}
					}
				}

				// now we want to swap?
				fltImg.swap(halfImg);
			}
		}


		return true;
	}

	bool ImgConveter::Convert(Texturefmt::Enum targetFmt)
	{
		if (targetFmt == srcImg_.getFormat()) {
			X_LOG1("Img", "Skipping texture conversion, src format matches target fmt of: %s",
				Texturefmt::ToString(targetFmt));
			useSrc_ = true;
			return true;
		}

		// input must be 32bit/pixel sRGB
		// for HDR is 64bit/pixel half floats.
		if (targetFmt == Texturefmt::BC7 || targetFmt == Texturefmt::BC3 || targetFmt == Texturefmt::BC1)
		{
			if (srcImg_.getFormat() != Texturefmt::R8G8B8A8 && srcImg_.getFormat() != Texturefmt::B8G8R8A8)
			{
				X_ERROR("Img", "Converting to %s requires R8G8B8A8 src. have: %s", 
					Texturefmt::ToString(targetFmt), Texturefmt::ToString(srcImg_.getFormat()));
				return false;
			}
		}
		else if (targetFmt == Texturefmt::BC6)
		{
			if (srcImg_.getFormat() != Texturefmt::R16G16B16A16_FLOAT)
			{
				X_ERROR("Img", "Converting to %s requires R16G16B16A16 src. have: %s", 
					Texturefmt::ToString(targetFmt), Texturefmt::ToString(srcImg_.getFormat()));
				return false;
			}
		}
		else
		{
			X_ERROR("Img", "Converting to %s not currently supported", Texturefmt::ToString(targetFmt));
			return false;
		}


		if (srcImg_.getDepth() > 1) {
			X_ERROR("Img", "Converting cube images is not supported");
			return false;
		}


		dstImg_.setSize(srcImg_.getSize());
		dstImg_.setFormat(targetFmt);
		dstImg_.setType(srcImg_.getType());
		dstImg_.setNumMips(srcImg_.getNumMips());
		// no cube or vol at no.
		dstImg_.setNumFaces(srcImg_.getNumFaces());
		dstImg_.setDepth(1);
		dstImg_.resize();

		Vec2<uint16_t> size = srcImg_.getSize();

		// time to convert a pickle.
		// we will need some sort of profile selection.
		// i guess that should be a arg, as to the quality level.
		ispc::bc7_enc_settings settingsbc7;
		ispc::GetProfile_veryfast(&settingsbc7);
		ispc::bc6h_enc_settings settingsbc6;
		ispc::GetProfile_bc6h_veryfast(&settingsbc6);

		for (size_t faceIdx = 0; faceIdx < srcImg_.getNumFaces(); faceIdx++)
		{
			for (size_t i = 0; i < srcImg_.getNumMips(); i++)
			{
				// for each mip
				ispc::rgba_surface inputImg;
				inputImg.ptr = srcImg_.getLevel(faceIdx, i);
				inputImg.width = size.x;
				inputImg.height = size.y;
				inputImg.stride = safe_static_cast<uint32_t, size_t>(Util::rowBytes(size.x, size.y, srcImg_.getFormat()));

				uint8_t* pOut = dstImg_.getLevel(faceIdx, i);

				if (targetFmt == Texturefmt::BC7)
				{
					ispc::CompressBlocksBC7(&inputImg, pOut, &settingsbc7);
				}
				else if (targetFmt == Texturefmt::BC6)
				{
					ispc::CompressBlocksBC6H(&inputImg, pOut, &settingsbc6);
				}
				else if (targetFmt == Texturefmt::BC3)
				{
					ispc::CompressBlocksBC3(&inputImg, pOut);
				}
				else if (targetFmt == Texturefmt::BC1)
				{
					ispc::CompressBlocksBC1(&inputImg, pOut);
				}
				else
				{
					// you twat!
					X_ASSERT_NOT_IMPLEMENTED();
				}

				size.x >>= 1;
				size.y >>= 1;
			}
		}

		return true;
	}

	const XTextureFile& ImgConveter::getTextFile(void) const
	{
		return srcImg_;
	}

	bool ImgConveter::ParseImgFmt(const char* pImgFmt, Texturefmt::Enum& fmtOut)
	{
		fmtOut = Texturefmt::UNKNOWN;

		core::StackString<128> imgFmt(pImgFmt);
		imgFmt.toUpper();

		// everything is 2 or mroe currently.
		if (imgFmt.length() < 2) {
			return false;
		}

		const core::StrHash nameHash(imgFmt.begin(), imgFmt.length());
		const size_t numFmts = sizeof(formats) / sizeof(FmtStr);

		for (size_t i = 0; i < numFmts; i++)
		{
			const auto& fmt = formats[i];

			if (fmt.nameHash == nameHash)
			{
				if (imgFmt.isEqual(fmt.pName))
				{
					fmtOut = fmt.fmt;
					return true;
				}
			}
		}

		return false;
	}

	void ImgConveter::getDefaultFilterWidthAndParams(MipFilter::Enum filter, MipMapFilterParams& params)
	{
		if (filter == MipFilter::Box) {
			params.filterWidth = 0.5f;
			params.params[0] = 0.f;
			params.params[1] = 0.f;
		}
		else if (filter == MipFilter::Triangle) {
			params.filterWidth = 1.0f;
			params.params[0] = 0.f;
			params.params[1] = 0.f;
		}
		else if (filter == MipFilter::Kaiser)
		{
			params.filterWidth = 3.0f;
			params.params[0] = 4.0f;
			params.params[1] = 1.0f;
		}
		else {
			X_ASSERT_NOT_IMPLEMENTED();
		}
	}


	ITextureFmt* ImgConveter::Allocfmt(core::LinearAllocator* pAllocator, ImgFileFormat::Enum inputFileFmt)
	{
		ITextureFmt* pFmt = nullptr;

		switch (inputFileFmt)
		{
		case ImgFileFormat::DDS:
			pFmt = core::Mem::Construct<DDS::XTexLoaderDDS>(pAllocator->allocate(sizeof(DDS::XTexLoaderDDS), X_ALIGN_OF(DDS::XTexLoaderDDS), 0));
			break;
		case ImgFileFormat::CI:
			pFmt = core::Mem::Construct<CI::XTexLoaderCI>(pAllocator->allocate(sizeof(CI::XTexLoaderCI), X_ALIGN_OF(CI::XTexLoaderCI), 0));
			break;
		case ImgFileFormat::JPG:
			pFmt = core::Mem::Construct<JPG::XTexLoaderJPG>(pAllocator->allocate(sizeof(JPG::XTexLoaderJPG), X_ALIGN_OF(JPG::XTexLoaderJPG), 0));
			break;
		case ImgFileFormat::TGA:
			pFmt = core::Mem::Construct<TGA::XTexLoaderTGA>(pAllocator->allocate(sizeof(TGA::XTexLoaderTGA), X_ALIGN_OF(TGA::XTexLoaderTGA), 0));
			break;
		case ImgFileFormat::PSD:
			pFmt = core::Mem::Construct<PSD::XTexLoaderPSD>(pAllocator->allocate(sizeof(PSD::XTexLoaderPSD), X_ALIGN_OF(PSD::XTexLoaderPSD), 0));
			break;
		case ImgFileFormat::PNG:
			pFmt = core::Mem::Construct<PNG::XTexLoaderPNG>(pAllocator->allocate(sizeof(PNG::XTexLoaderPNG), X_ALIGN_OF(PNG::XTexLoaderPNG), 0));
			break;
		}

		return pFmt;
	}

} // namespace Converter




X_NAMESPACE_END