#include "stdafx.h"
#include "Converter.h"

#include <IFileSys.h>

#include <Threading\JobSystem2.h>

#include <String\StringHash.h>

#include <Memory/AllocationPolicies/LinearAllocator.h>

#include "Util\Filters.h"
#include "Util\FloatImage.h"


X_NAMESPACE_BEGIN(texture)


namespace Converter
{


	namespace
	{

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


	ImgConveter::ImgConveter(core::MemoryArenaBase* imgArena, core::MemoryArenaBase* swapArena) :
		swapArena_(swapArena),
		srcImg_(imgArena),
		dstImg_(imgArena),
		useSrc_(true),
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

		return Util::loadImage(swapArena_, fileData, inputFileFmt, srcImg_);
	}

	bool ImgConveter::LoadImg(core::XFile* pFile, ImgFileFormat::Enum inputFileFmt)
	{
		if (inputFileFmt == ImgFileFormat::UNKNOWN) {
			X_ERROR("Img", "Failed to load img srcFmt unkNown");
			return false;
		}

		useSrc_ = true;

		return Util::loadImage(swapArena_, pFile, inputFileFmt, srcImg_);
	}

	bool ImgConveter::SaveImg(const core::Path<char>& outPath, CompileFlags flags, ImgFileFormat::Enum dstFileFmt)
	{
		core::fileModeFlags mode;
		mode.Set(core::fileMode::WRITE);
		mode.Set(core::fileMode::RECREATE);

		core::XFileScoped file;
		if (!file.openFile(outPath.c_str(), mode)) {
			return false;
		}

		return SaveImg(file.GetFile(), flags, dstFileFmt);
	}

	bool ImgConveter::SaveImg(core::XFile* pFile, CompileFlags flags, ImgFileFormat::Enum dstFileFmt)
	{
		if (!Util::writeSupported(dstFileFmt)) {
			X_ERROR("Img", "Writing to fmt: %s is not supported", ImgFileFormat::ToString(dstFileFmt));
			return false;
		}

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


		const bool res = Util::saveImage(swapArena_, pFile, dstFileFmt, img);

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
		case ScaleFactor::FOURTH:
			srcImg_.dropTopMip();
		case ScaleFactor::HALF:
			srcImg_.dropTopMip();
			break;
		default:
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
			pRootJob = jobSys.CreateJob(core::V2::JobSystem::EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));

			bool results[TEX_MAX_FACES] = { false };

			for (int32_t face = 0; face < srcImg_.getNumFaces(); face++)
			{
				MipFaceJobData jobData = { swapArena_, srcImg_, params, filter, wrap, alpha, results[face], face };

				core::V2::Job* pJob = jobSys.CreateJobAsChild(pRootJob, generateMipsJob, jobData JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
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

			if (!halfImg.saveToImg(srcImg, faceIdx, mip)) {
				X_ERROR("Img", "Error Saving mip data to src format");
				jobdata.result = false;
				return;
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
		else {
			useSrc_ = false;
		}

		// input must be 32bit/pixel sRGB
		// for HDR is 64bit/pixel half floats.
		if (targetFmt == Texturefmt::BC7 || targetFmt == Texturefmt::BC3 || targetFmt == Texturefmt::BC1)
		{
			// if we brg8 need to switch channels.
			if (srcImg_.getFormat() == Texturefmt::B8G8R8A8)
			{
				// where is the best place to put this channel flip logic?
				// right here, seams like best place to call it, but should it be impl in util?
				for (size_t face = 0; face < srcImg_.getNumFaces(); face++)
				{
					for (size_t mip = 0; mip < srcImg_.getNumMips(); mip++)
					{
						uint8_t* pSrc = srcImg_.getLevel(face, mip);
						const size_t numPixel = srcImg_.getLevelSize(mip) / 4;

						for (size_t i = 0; i < numPixel; i++)
						{
							std::swap(pSrc[0], pSrc[2]);
							pSrc += 4;
						}
					}
				}

				srcImg_.setFormat(Texturefmt::R8G8B8A8);
			}

			if (srcImg_.getFormat() != Texturefmt::R8G8B8A8)
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
		else if (targetFmt == Texturefmt::R8G8B8A8)
		{
			if (srcImg_.getFormat() != Texturefmt::R8G8B8A8 && srcImg_.getFormat() != Texturefmt::B8G8R8A8)
			{
				X_ERROR("Img", "Converting to %s requires B8G8R8A8 or R8G8B8A8 src. have: %s",
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


		if (targetFmt == Texturefmt::R8G8B8A8)
		{
			X_ASSERT(srcImg_.getFormat() == Texturefmt::B8G8R8A8, "Unexpceted src format")();

			for (size_t face = 0; face < srcImg_.getNumFaces(); face++)
			{
				for (size_t mip = 0; mip < srcImg_.getNumMips(); mip++)
				{
					uint8_t* pSrc = srcImg_.getLevel(face, mip);
					uint8_t* pDst = dstImg_.getLevel(face, mip);
					
					const size_t numPixel = srcImg_.getLevelSize(mip) / 4;

					for (size_t i = 0; i < numPixel; i++)
					{
						pDst[0] = pSrc[2];
						pDst[1] = pSrc[1];
						pDst[2] = pSrc[0];
						pDst[3] = pSrc[3];

						pSrc += 4;
						pDst += 4;
					}
				}
			}

			return true;
		}


		core::V2::JobSystem& jobSys = *gEnv->pJobSys;
		core::V2::Job* pRootJob = nullptr;

		if (multiThread_) 
		{
			pRootJob = jobSys.CreateJob(core::V2::JobSystem::EmptyJob JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
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

				auto subBlockConvert = [&]() {
					X_ASSERT(inputImg.height < 4, "Height should be below 4")(inputImg.height);

					// we need to pad this up into a 4x4 block for converting.
					// we should fill that pixels we don't plan to use with real pixels
					// that way the block format is able to make a better pallet.
					std::array<uint8_t, 4> block[4 * 4];

					for (int32_t y = 0; y < 4; y++)
					{
						for (int32_t x = 0; x <4; x++)
						{
							int32_t srcY = core::Min(y, inputImg.height);
							int32_t srcX = core::Min(x, inputImg.width);
							uint8_t* pSrcPixel = &inputImg.ptr[srcX * srcY];

							auto& dest = block[(y * 4) + x];

							std::memcpy(dest.data(), pSrcPixel, 4);
						}
					}

					ispc::rgba_surface surface;
					surface.ptr = block[0].data();
					surface.width = 4;
					surface.height = 4;
					surface.stride = 4 * 4;
					pFunc(&surface, pOut);
				};

				if (pRootJob)
				{
					const int32_t targetJobCount = 32;
					const int32_t rowsPerJob = (((inputImg.height / 4) + targetJobCount - 1) / targetJobCount) * 4;
					const int32_t bytesPerBlock = Util::dxtBytesPerBlock(targetFmt);
				//	const int32_t numJobs = core::Min(targetJobCount, inputImg.height);

					X_ASSERT((rowsPerJob % 4) == 0, "Rows to process should be multiple of 4")(rowsPerJob);

					if (!core::bitUtil::IsPowerOfTwo(inputImg.height) || !core::bitUtil::IsPowerOfTwo(inputImg.height)) {
						X_ASSERT_NOT_IMPLEMENTED();
					}

					if (rowsPerJob == 0)
					{
						subBlockConvert();
					}
					else
					{
						int32_t currentRow = 0;

						while (currentRow < inputImg.height)
						{
							int32_t y_start = currentRow;
							int32_t y_end = currentRow + rowsPerJob;

							// start + rows might bexceed height
							if (y_end > inputImg.height) {
								y_end = inputImg.height;
							}

							const int32_t numRowsToProcess = y_end - y_start;

							X_ASSERT(numRowsToProcess > 0, "Invalid range")(y_end, y_start, numRowsToProcess);
							X_ASSERT((numRowsToProcess % 4) == 0, "Rows to process should be multiple of 4")(numRowsToProcess);
							X_ASSERT((y_start % 4) == 0, "Start row should be a multiple of 4")(y_start);
							X_ASSERT((y_end % 4) == 0, "End row should be a multiple of 4")(y_end);

							JobData data;
							data.pCompressFunc = pFunc;
							data.surface.ptr = inputImg.ptr + y_start * inputImg.stride;
							data.surface.width = inputImg.width;
							data.surface.height = numRowsToProcess;
							data.surface.stride = inputImg.stride;
							data.pOut = pOut + (y_start / 4) * (inputImg.width / 4) * bytesPerBlock;

							currentRow += numRowsToProcess;

							core::V2::Job* pJob = jobSys.CreateJobAsChild(pRootJob, compressJob, data JOB_SYS_SUB_ARG(core::profiler::SubSys::TOOL));
							jobSys.Run(pJob);
						}
					}
				}
				else
				{
					if (inputImg.height < 4)
					{
						subBlockConvert();
					}
					else
					{
						pFunc(&inputImg, pOut);
					}
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

		X_ASSERT(pJobData->surface.height >= 1, "Invalid height")(pJobData->surface.height);
		X_ASSERT(pJobData->surface.width >= 1, "Invalid width")(pJobData->surface.width);

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


} // namespace Converter




X_NAMESPACE_END