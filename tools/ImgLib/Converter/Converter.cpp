#include "stdafx.h"
#include "Converter.h"

#include <Threading\JobSystem2.h>

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

		#define declareBC7_profile_helper(profile) \
		void CompressBlocksBC7_ ## profile(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut) \
		{ \
			ispc::bc7_enc_settings settingsbc7; \
			ispc::GetProfile_ ## profile(&settingsbc7); \
			ispc::CompressBlocksBC7(pSrcSurface, pOut, &settingsbc7); \
		}

		#define declareBC6H_profile_helper(profile) \
		void CompressBlocksBC6_ ## profile(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut) \
		{ \
			ispc::bc6h_enc_settings settingsbc6; \
			ispc::GetProfile_bc6h_ ## profile(&settingsbc6); \
			ispc::CompressBlocksBC6H(pSrcSurface, pOut, &settingsbc6); \
		}

		// bc7
		declareBC7_profile_helper(ultrafast);
		declareBC7_profile_helper(veryfast);
		declareBC7_profile_helper(fast);
		declareBC7_profile_helper(basic);
		declareBC7_profile_helper(slow);
		declareBC7_profile_helper(alpha_ultrafast);
		declareBC7_profile_helper(alpha_veryfast);
		declareBC7_profile_helper(alpha_fast);
		declareBC7_profile_helper(alpha_basic);
		declareBC7_profile_helper(alpha_slow);

		// bc6
		declareBC6H_profile_helper(veryfast);
		declareBC6H_profile_helper(fast);
		declareBC6H_profile_helper(basic);
		declareBC6H_profile_helper(slow);
		declareBC6H_profile_helper(veryslow);

		void CompressBlocksBC3(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut)
		{
			ispc::CompressBlocksBC3(pSrcSurface, pOut);
		}

		void CompressBlocksBC1(const ispc::rgba_surface* pSrcSurface, uint8_t* pOut)
		{
			ispc::CompressBlocksBC1(pSrcSurface, pOut);
		}


	} // namespace



	static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");


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
		useSrc_(false),
		multiThread_(true)
	{

	}

	ImgConveter::~ImgConveter()
	{

	}

	void ImgConveter::enableMultiThreaded(bool enable)
	{
		multiThread_ = enable;
	}

	bool ImgConveter::LoadImg(const core::Array<uint8_t>& fileData, ImgFileFormat::Enum inputFileFmt)
	{
		// check in here also even thos it's checked again, logic might change later to not use function below.
		if (inputFileFmt == ImgFileFormat::UNKNOWN) {
			X_ERROR("Img", "Failed to load img srcFmt unkNown");
			return false;
		}

		core::XFileFixedBuf file(fileData.begin(), fileData.end());

		return LoadImg(&file, inputFileFmt);
	}

	bool ImgConveter::LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt)
	{
		if (inputFileFmt == ImgFileFormat::UNKNOWN) {
			X_ERROR("Img", "Failed to load img srcFmt unkNown");
			return false;
		}

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

	void ImgConveter::scale(ScaleFactor::Enum scale)
	{
		if (scale == ScaleFactor::ORIGINAL) {
			return;
		}

		// I intentially fall through.
		// so scale Eight results in dropTop mipb been called 3 times.
		switch (scale)
		{
		case ScaleFactor::EIGHTH:
			srcImg_.dropTopMip();
		case ScaleFactor::QUARTER:
			srcImg_.dropTopMip();
		case ScaleFactor::HALF:
			srcImg_.dropTopMip();
			break;
		}
	}

	bool ImgConveter::addAlphachannel(bool keepMips)
	{
		// expand the src img to have a alpha channel.
		// needed for converting even if we don't use it :(
		if (srcImg_.getFormat() != Texturefmt::R8G8B8 && srcImg_.getFormat() != Texturefmt::B8G8R8)
		{
			X_ERROR("Img", "Generating alpha channels for format \"%s\" not currently supported",
				Texturefmt::ToString(srcImg_.getFormat()));
			return false;
		}

		// OK my fat little goat.
		// we need to allocate new buffer and copy data into it ;(
		XTextureFile tmp(swapArena_);
		
		tmp.setFormat(Texturefmt::R8G8B8A8);
		tmp.setType(srcImg_.getType());
		tmp.setHeigth(srcImg_.getHeight());
		tmp.setWidth(srcImg_.getWidth());
		tmp.setNumFaces(srcImg_.getNumFaces());
		tmp.setDepth(1);
		tmp.setNumMips(1);

		if (keepMips) {
			tmp.setNumMips(srcImg_.getNumMips());
		}

		tmp.resize();

		for (size_t face = 0; face < tmp.getNumFaces(); face++)
		{
			for (size_t mip = 0; mip < tmp.getNumMips(); mip++)
			{
				uint8_t* pDest = tmp.getLevel(face, mip);
				const uint8_t* pSrc = srcImg_.getLevel(face, mip);

				// 3 bytes per pixel.
				const size_t numPixel = srcImg_.getLevelSize(mip) / 3;

				if (srcImg_.getFormat() == Texturefmt::R8G8B8)
				{
					for (size_t i = 0; i < numPixel; i++)
					{
						pDest[0] = pSrc[0];
						pDest[1] = pSrc[1];
						pDest[2] = pSrc[2];
						pDest[3] = 0xFF;

						pSrc += 3;
						pDest += 4;
					}
				}
				else if (srcImg_.getFormat() == Texturefmt::B8G8R8)
				{
					for (size_t i = 0; i < numPixel; i++)
					{
						// swap the channels
						pDest[0] = pSrc[2];
						pDest[1] = pSrc[1];
						pDest[2] = pSrc[0];
						pDest[3] = 0xFF;

						pSrc += 3;
						pDest += 4;
					}
				}
				else
				{
					X_ASSERT_NOT_IMPLEMENTED();
					return false;
				}
			}
		}

		// swap.
		srcImg_.swap(tmp);
		return true;
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
		srcImg_.allocMipBuffers();
		X_ASSERT(srcImg_.getNumMips() == requiredMips, "Mip count mismatch")(srcImg_.getNumMips(), requiredMips);

		core::V2::JobSystem& jobSys = *gEnv->pJobSys;
		core::V2::Job* pRootJob = nullptr;

		if (multiThread_) 
		{
			pRootJob = jobSys.CreateJob(core::V2::JobSystem::EmptyJob);

			bool results[TEX_MAX_FACES] = { false };

			for (int32_t face = 0; face < srcImg_.getNumFaces(); face++)
			{
				MipFaceJobData jobData = { swapArena_, srcImg_, params, filter, wrap, alpha, results[face], face };

				core::V2::Job* pJob = jobSys.CreateJobAsChild(pRootJob, generateMipsJob, jobData);
				jobSys.Run(pJob);
			}

			jobSys.Run(pRootJob);
			jobSys.Wait(pRootJob);

			for (int32_t face = 0; face < srcImg_.getNumFaces(); face++) {
				if (!results[face]) {
					return false;
				}
			}
		}
		else
		{
			for (int32_t face = 0; face < srcImg_.getNumFaces(); face++)
			{
				bool result = false;
				MipFaceJobData jobData = { swapArena_, srcImg_, params, filter, wrap, alpha, result, face };
				generateMipsForFace(jobData);

				if (!result) {
					return false;
				}
			}
		}

		return true;
	}



	void ImgConveter::generateMipsJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
	{
		X_UNUSED(jobSys);
		X_UNUSED(threadIdx);
		X_UNUSED(pJob);

		MipFaceJobData* pJobData = reinterpret_cast<MipFaceJobData*>(pData);

		generateMipsForFace(*pJobData);
	}

	void ImgConveter::generateMipsForFace(MipFaceJobData& jobdata)
	{
		FloatImage fltImg(jobdata.swapArena);
		FloatImage halfImg(jobdata.swapArena);

		core::MemoryArenaBase* swapArena = jobdata.swapArena;
		XTextureFile& srcImg = jobdata.srcImg;
		const MipFilter::Enum filter = jobdata.filter;
		const MipMapFilterParams& params = jobdata.params;
		const WrapMode::Enum wrap = jobdata.wrap;
		const int32_t faceIdx = jobdata.faceIdx;

		const uint32_t requiredMips = Util::maxMipsForSize(srcImg.getWidth(), srcImg.getHeight());

		// create from first mip and level
		if (!fltImg.initFrom(srcImg, faceIdx, 0)) {
			X_ERROR("Img", "Error loading src for mip generation");
			jobdata.result = false;
			return;
		}

		for (uint32_t mip = 1; mip < requiredMips; mip++)
		{
			if (jobdata.alpha)
			{
				if (filter == MipFilter::Box)
				{
					BoxFilter filter(params.filterWidth);
					fltImg.downSample(halfImg, swapArena, filter, wrap, 3);
				}
				else if (filter == MipFilter::Triangle)
				{
					TriangleFilter filter(params.filterWidth);
					fltImg.downSample(halfImg, swapArena, filter, wrap, 3);
				}
				else if (filter == MipFilter::Kaiser)
				{
					KaiserFilter filter(params.filterWidth);
					filter.setParameters(params.params[0], params.params[1]);
					fltImg.downSample(halfImg, swapArena, filter, wrap, 3);
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
					if (math<float>::abs(params.filterWidth - 0.5f) < EPSILON && srcImg.getNumFaces() == 1) {
						fltImg.fastDownSample(halfImg);
					}
					else {
						BoxFilter filter(params.filterWidth);
						fltImg.downSample(halfImg, swapArena, filter, wrap);
					}
				}
				else if (filter == MipFilter::Triangle)
				{
					TriangleFilter filter(params.filterWidth);
					fltImg.downSample(halfImg, swapArena, filter, wrap);
				}
				else if (filter == MipFilter::Kaiser)
				{
					KaiserFilter filter(params.filterWidth);
					filter.setParameters(params.params[0], params.params[1]);
					fltImg.downSample(halfImg, swapArena, filter, wrap);
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

			// we copy the data back to src img.
			{
				const size_t dstSize = srcImg.getLevelSize(mip);
				uint8_t* pMipDst = srcImg.getLevel(faceIdx, mip);

				if (srcImg.getFormat() == Texturefmt::R16G16B16A16_FLOAT)
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
				else if (srcImg.getFormat() == Texturefmt::R8G8B8A8)
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
				else if (srcImg.getFormat() == Texturefmt::B8G8R8A8)
				{
					const size_t expectedSize = (numPixels * 4);
					X_ASSERT(dstSize == expectedSize, "Size missmatch for expected vs provided size")(dstSize, expectedSize);

					for (uint32_t i = 0; i < numPixels; i++)
					{
						uint8_t* pPixel = &pMipDst[i * 4];
						pPixel[0] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pBlueChannel[i], 0.f, 1.f));
						pPixel[1] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pGreenChannel[i], 0.f, 1.f));
						pPixel[2] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pRedChannel[i], 0.f, 1.f));
						pPixel[3] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pAlphaChannel[i], 0.f, 1.f));
					}
				}
				else if (srcImg.getFormat() == Texturefmt::R8G8B8)
				{
					const size_t expectedSize = (numPixels * 3);
					X_ASSERT(dstSize == expectedSize, "Size missmatch for expected vs provided size")(dstSize, expectedSize);

					for (uint32_t i = 0; i < numPixels; i++)
					{
						uint8_t* pPixel = &pMipDst[i * 3];
						pPixel[0] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pRedChannel[i], 0.f, 1.f));
						pPixel[1] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pGreenChannel[i], 0.f, 1.f));
						pPixel[2] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pBlueChannel[i], 0.f, 1.f));
					}
				}
				else if (srcImg.getFormat() == Texturefmt::B8G8R8)
				{
					const size_t expectedSize = (numPixels * 3);
					X_ASSERT(dstSize == expectedSize, "Size missmatch for expected vs provided size")(dstSize, expectedSize);

					for (uint32_t i = 0; i < numPixels; i++)
					{
						uint8_t* pPixel = &pMipDst[i * 3];
						pPixel[0] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pBlueChannel[i], 0.f, 1.f));
						pPixel[1] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pGreenChannel[i], 0.f, 1.f));
						pPixel[2] = static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() * math<float>::clamp(pRedChannel[i], 0.f, 1.f));
					}
				}
				else
				{
					X_ASSERT_NOT_IMPLEMENTED();
					jobdata.result = false;
					return;
				}
			}

			// now we want to swap?
			fltImg.swap(halfImg);
		}


		jobdata.result = true;		
	}

	bool ImgConveter::Convert(Texturefmt::Enum targetFmt, Profile::Enum profile, bool keepAlpha)
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
		dstImg_.setNumFaces(srcImg_.getNumFaces());
		dstImg_.setDepth(1);
		dstImg_.resize();

		core::V2::JobSystem& jobSys = *gEnv->pJobSys;
		core::V2::Job* pRootJob = nullptr;

		if (multiThread_) 
		{
			pRootJob = jobSys.CreateJob(core::V2::JobSystem::EmptyJob);
		}

		CompressionFunc::Pointer pFunc = getCompressionFunc(targetFmt, profile, keepAlpha);

		for (size_t faceIdx = 0; faceIdx < srcImg_.getNumFaces(); faceIdx++)
		{
			Vec2<uint16_t> size = srcImg_.getSize();

			for (size_t i = 0; i < srcImg_.getNumMips(); i++)
			{
				// for each mip
				ispc::rgba_surface inputImg;
				inputImg.ptr = srcImg_.getLevel(faceIdx, i);
				inputImg.width = size.x;
				inputImg.height = size.y;
				inputImg.stride = safe_static_cast<uint32_t, size_t>(Util::rowBytes(size.x, size.y, srcImg_.getFormat()));

				uint8_t* pOut = dstImg_.getLevel(faceIdx, i);

				if (pRootJob)
				{
					const int32_t numJobs = 32;
					const int32_t linesPerJob = (inputImg.height + numJobs - 1) / numJobs;
					const int32_t bytesPerBlock = Util::dxtBytesPerBlock(targetFmt);

					// add job for each row?
					for(int32_t jobIdx = 0; jobIdx < numJobs; jobIdx++)
					{
						const int32_t y_start = (linesPerJob*jobIdx) / 4 * 4;
						int32_t y_end = (linesPerJob*(jobIdx + 1)) / 4 * 4;
						if (y_end > inputImg.height) {
							y_end = inputImg.height;
						}

						if ((y_end - y_start) == 0) {
							continue;
						}

						JobData data;
						data.pCompressFunc = pFunc;
						data.surface.ptr = inputImg.ptr + y_start * inputImg.stride;
						data.surface.width = inputImg.width;
						data.surface.height = y_end - y_start;
						data.surface.stride = inputImg.stride;
						data.pOut = pOut + (y_start / 4) * (inputImg.width / 4) * bytesPerBlock;

						core::V2::Job* pJob = jobSys.CreateJobAsChild(pRootJob, compressJob, data);
						jobSys.Run(pJob);
					}
				}
				else
				{
					pFunc(&inputImg, pOut);
				}

				size.x >>= 1;
				size.y >>= 1;
			}
		}

		if (pRootJob)
		{
			// wait for every mip to complete.
			jobSys.Run(pRootJob);
			jobSys.Wait(pRootJob);
		}

		return true;
	}


	void ImgConveter::compressJob(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
	{
		X_UNUSED(jobSys);
		X_UNUSED(threadIdx);
		X_UNUSED(pJob);

		JobData* pJobData = reinterpret_cast<JobData*>(pData);

		pJobData->pCompressFunc(&pJobData->surface, pJobData->pOut);
	}

	ImgConveter::CompressionFunc::Pointer ImgConveter::getCompressionFunc(Texturefmt::Enum fmt, Profile::Enum profile, bool keepAlpha)
	{
		X_UNUSED(keepAlpha);

		if (fmt == Texturefmt::BC7)
		{
			if (keepAlpha)
			{
				switch (profile)
				{
				case Profile::UltraFast:
					return CompressBlocksBC7_alpha_ultrafast;
				case Profile::VeryFast:
					return CompressBlocksBC7_alpha_veryfast;
				case Profile::Fast:
					return CompressBlocksBC7_alpha_fast;
				case Profile::Basic:
					return CompressBlocksBC7_alpha_basic;
				case Profile::Slow:
					return CompressBlocksBC7_alpha_slow;
				default:
					X_ASSERT_UNREACHABLE();
					return nullptr;
				}
			}

			switch (profile)
			{
			case Profile::UltraFast:
				return CompressBlocksBC7_ultrafast;
			case Profile::VeryFast:
				return CompressBlocksBC7_veryfast;
			case Profile::Fast:
				return CompressBlocksBC7_fast;
			case Profile::Basic:
				return CompressBlocksBC7_basic;
			case Profile::Slow:
				return CompressBlocksBC7_slow;
			default:
				X_ASSERT_UNREACHABLE();
				return nullptr;
			}
		}
		else if (fmt == Texturefmt::BC6)
		{
			// the bc6 ones have diffrent naming but same count so i've shiffted them up.
			// veryslow is now slow and slow is basic etc.
			switch (profile)
			{
			case Profile::UltraFast:
				return CompressBlocksBC6_veryfast; 
			case Profile::VeryFast:
				return CompressBlocksBC6_fast;
			case Profile::Fast:
				return CompressBlocksBC6_basic;
			case Profile::Basic:
				return CompressBlocksBC6_slow;
			case Profile::Slow:
				return CompressBlocksBC6_veryslow;
			default:
				X_ASSERT_UNREACHABLE();
				return nullptr;
			}
		}
		else if (fmt == Texturefmt::BC3)
		{
			return CompressBlocksBC3;
		}
		else if (fmt == Texturefmt::BC1)
		{
			return CompressBlocksBC1;
		}

		X_ASSERT_UNREACHABLE();
		return nullptr;
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

		static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img fmts? this code needs updating.");

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

		default:
			X_ASSERT_UNREACHABLE();
			break;
		}

		return pFmt;
	}

} // namespace Converter




X_NAMESPACE_END