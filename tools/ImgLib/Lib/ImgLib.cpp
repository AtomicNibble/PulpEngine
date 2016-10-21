#include "stdafx.h"
#include "ImgLib.h"

#include <Time\StopWatch.h>

#include <String\StringHash.h>
#include "Converter\Converter.h"

#include <ICi.h>





X_NAMESPACE_BEGIN(texture)

using namespace Converter;

ImgLib::ImgLib()
{

}

ImgLib::~ImgLib()
{

}

const char* ImgLib::getOutExtension(void) const
{
	return texture::CI_FILE_EXTENSION;
}

bool ImgLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	core::json::Document d;
	d.Parse(args.c_str());

	CompileFlags flags;
	ScaleFactor::Enum scale = ScaleFactor::ORIGINAL;
	ImgFileFormat::Enum outputFileFmt = ImgFileFormat::CI;
	MipFilter::Enum mipFilter = MipFilter::Box;
	WrapMode::Enum wrapMode = WrapMode::Clamp;
	Texturefmt::Enum dstImgFmt = Texturefmt::UNKNOWN;

	// Required args:
	// * imgPath
	// * imgFmt (for now)
	// 

	if (d.HasMember("ignoreSrcMips")) {
		if (d["ignoreSrcMips"].GetBool()) {
			flags.Set(CompileFlag::IGNORE_SRC_MIPS);
		}
	}
	if (d.HasMember("noMips")) {
		if (d["noMips"].GetBool()) {
			flags.Set(CompileFlag::NOMIPS);
		}
	}
	if (d.HasMember("preMulAlpha")) {
		if (d["preMulAlpha"].GetBool()) {
			flags.Set(CompileFlag::PREMULTIPLY_ALPHA);
		}
	}
	if (d.HasMember("nonePow2")) {
		if (d["nonePow2"].GetBool()) {
			flags.Set(CompileFlag::ALLOW_NONE_POW2);
		}
	}
	if (d.HasMember("alpha")) {
		if (d["alpha"].GetBool()) {
			flags.Set(CompileFlag::ALPHA);
		}
	}
	if (d.HasMember("mipFilter")) {
		const char* pMipFilter = d["mipFilter"].GetString();

		if (core::strUtil::IsEqualCaseInsen(pMipFilter, "Box")) {
			mipFilter = MipFilter::Box;
		}
		else if (core::strUtil::IsEqualCaseInsen(pMipFilter, "Triangle")) {
			mipFilter = MipFilter::Triangle;
		}
		else if (core::strUtil::IsEqualCaseInsen(pMipFilter, "Kaiser")) {
			mipFilter = MipFilter::Kaiser;
		}
		else {
			X_ERROR("Img", "Invalid mipfilter: \"%s\"", pMipFilter);
			return false;
		}
	}

	if (d.HasMember("scale")) {
		const char* pScaleFactor = d["scale"].GetString();

		if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/1")) {
			scale = ScaleFactor::ORIGINAL;
		} else if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/2")) {
			scale = ScaleFactor::HALF;
		}
		else if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/4")) {
			scale = ScaleFactor::QUARTER;
		}
		else if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/8")) {
			scale = ScaleFactor::EIGHTH;
		}
		else {
			X_ERROR("Img", "Invalid scale factor: \"%s\"", pScaleFactor);
			return false;
		}	
	}
	if (d.HasMember("outFmt")) {
		const char* pOutFmt = d["outFmt"].GetString();

		if (!core::strUtil::IsEqualCaseInsen(pOutFmt, "ci")) {
			X_ERROR("Img", "Ouput file fmt currently not supported: \"%s\"", pOutFmt);
			return false;
		}

		// if the need arises we can support writting to a DDS
		// or something else...
		outputFileFmt = ImgFileFormat::CI;
	}
	if (d.HasMember("wrap")) {
		const char* pWrap = d["wrap"].GetString();

		if (!core::strUtil::IsEqualCaseInsen(pWrap, "Mirror")) {
			wrapMode = WrapMode::Mirror;
		} else if (!core::strUtil::IsEqualCaseInsen(pWrap, "Repeat")) {
			wrapMode = WrapMode::Repeat;
		} else if (!core::strUtil::IsEqualCaseInsen(pWrap, "Clamp")) {
			wrapMode = WrapMode::Clamp;
		} else {
			X_ERROR("Img", "Wrap mode not supported: \"%s\"", pWrap);
			return false;
		}
	}

	// we want to support converting to all the supported image formats.
	// which is basically the Texturefmt enum.
	// which I really can't be botherd to fully support for conversion.
	bool autoFmt = true;

	if (d.HasMember("compressionMethod"))
	{
		const auto& val = d["compressionMethod"];
		using namespace core::Hash::Fnva1Literals;

		/*
			Best color compression[compressHighCol]
			Better alpha compression[compressHighAlpha]
			Low quality color compression with no Alpha[compressCol]
			Uncompressed[noCompress]
			Custom[custom]
		*/

		switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength()))
		{
			case "compressHighCol"_fnv1a:
				dstImgFmt = Texturefmt::BC7;
				break;
			case "compressHighAlpha"_fnv1a:
				dstImgFmt = Texturefmt::BC3;
				break;
			case "compressCol"_fnv1a:
				dstImgFmt = Texturefmt::BC1;
				break;
			case "noCompress"_fnv1a:
				dstImgFmt = Texturefmt::R8G8B8A8;
				break;
			case "custom"_fnv1a:
				autoFmt = false;
				break;

			default:
				X_ERROR("Img", "Unknown compressionMethod: \"%.*s\"", val.GetStringLength(), val.GetString());
				return false;
		}
	}

	if (!autoFmt)
	{
		if (!d.HasMember("imgFmt")) {
			X_ERROR("Img", "imgFmt option required for custom compressionMethod");
			return false;
		}

		const char* pImgFmt = d["imgFmt"].GetString();

		if (!ImgConveter::ParseImgFmt(pImgFmt, dstImgFmt)) {
			X_ERROR("Img", "Unknown img fmt: \"%s\"", pImgFmt);
			return false;
		}
	}


	// now we check i support creating the target format.
	switch (dstImgFmt)
	{
		case Texturefmt::BC1:
		case Texturefmt::BC3:
		case Texturefmt::BC6:
		case Texturefmt::BC7:
		case Texturefmt::R8G8B8A8:
			break;

		default:
			X_ERROR("Img", "Format not implemented for conversion: \"%s\"", Texturefmt::ToString(dstImgFmt));
			return false;
	}

	// check we only got here with valid shit!
	X_ASSERT(outputFileFmt != ImgFileFormat::UNKNOWN, "OutputFile Fmt is invalid")(outputFileFmt);


	ImgConveter::Profile::Enum qualityProfile = ImgConveter::Profile::VeryFast;
	ImgConveter con(g_ImgLibArena, g_ImgLibArena);


	// we support global overrides.
	core::string conProfile;
	if (host.getConversionProfileData(assetDb::AssetType::IMG, conProfile))
	{
		core::json::Document pd;
		pd.Parse(conProfile.c_str());

		for (auto it = pd.MemberBegin(); it != pd.MemberEnd(); ++it)
		{
			const auto& name = it->name;
			const auto& val = it->value;

			using namespace core::Hash::Fnva1Literals;

			switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength()))
			{
				case "qualityProfile"_fnv1a:
					switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength()))
					{
						case "UltraFast"_fnv1a:
							qualityProfile = ImgConveter::Profile::UltraFast;
							break;
						case "VeryFast"_fnv1a:
							qualityProfile = ImgConveter::Profile::VeryFast;
							break;
						case "Fast"_fnv1a:
							qualityProfile = ImgConveter::Profile::Fast;
							break;
						case "Basic"_fnv1a:
							qualityProfile = ImgConveter::Profile::Basic;
							break;
						case "Slow"_fnv1a:
							qualityProfile = ImgConveter::Profile::Slow;
							break;
						default:
							X_WARNING("Img", "Unknown qualityProfile profile: %.*s", val.GetStringLength(), val.GetString());
							break;
					}
					break;

				case "scaleShift"_fnv1a:
					if (val.GetType() == core::json::Type::kNumberType)
					{
						auto shift = val.GetInt();

						// we support negative and positive shifts.
						// we flip the shift tho so -2 results in more downshifting.
						shift = -shift;

						// shift the scale and clamp to valid enum range.
						const int32_t scaleNumeric = math<int32_t>::clamp(static_cast<int32_t>(scale) + shift, 0, ScaleFactor::ENUM_COUNT - 1);

						scale = static_cast<ScaleFactor::Enum>(scaleNumeric);
					}
					break;

				case "scaleForce"_fnv1a:
					if (val.GetType() == core::json::Type::kNumberType)
					{
						const auto force = val.GetInt();
						const int32_t scaleNumeric = math<int32_t>::clamp(force, 0, ScaleFactor::ENUM_COUNT - 1);

						scale = static_cast<ScaleFactor::Enum>(scaleNumeric);
					}
					break;

				default:
					X_WARNING("Img", "Unknown conversion option: %.*s", name.GetStringLength(), name.GetString());
					break;
			}
		}
	}

	// load file data
	core::Array<uint8_t> fileData(host.getScratchArena());
	if (!host.GetAssetData(assetId, fileData)) {
		X_ERROR("Img", "Failed to get asset data");
		return false;
	}

	if (fileData.isEmpty()) {
		X_ERROR("Img", "File data is empty");
		return false;
	}

	// work out the fmt.
	ImgFileFormat::Enum inputFileFmt = Util::resolveSrcfmt(fileData);
	if (inputFileFmt == ImgFileFormat::UNKNOWN) {
		X_ERROR("Img", "Unknown img src format");
		return false;
	}
	if (inputFileFmt == ImgFileFormat::CI) {
		X_ERROR("Img", "Input format of type CI is not allowed");
		return false;
	}

	// load it :D
	if (!con.LoadImg(fileData, inputFileFmt)) {
		X_ERROR("Img", "Failed to load source image");
		return false;
	}

	{
		const auto& src = con.getTextFile();

		X_ASSERT(src.isValid(), "Src img not valid after successful loading")(src.isValid());

		if (!core::bitUtil::IsPowerOfTwo(src.getWidth()) ||
			!core::bitUtil::IsPowerOfTwo(src.getHeight()))
		{
			// none pow 2 img :| https://winpic.co/MAbde113843b.gif

			if (!flags.IsSet(CompileFlag::ALLOW_NONE_POW2))
			{
				X_ERROR("Img", "Image is not power of two: %" PRIi32 "x%" PRIi32, src.getWidth(), src.getHeight());
				return false;
			}

			// when allowing none pow2 images, we need to make sure block formats are processed correct.
			// throw this here for now.
			X_ASSERT_NOT_IMPLEMENTED();
			return false;
		}

		if (flags.IsSet(CompileFlag::NO_COMPRESSION))
		{
			// you want none compressed but gave me a block format, fuck you!
			if (Util::isDxt(src.getFormat()))
			{
				X_ERROR("Img", "No compression requested, but source format(%s) is already block compressed. bitch please..",
					Texturefmt::ToString(src.getFormat()));
				return false;
			}

			// just leave as src format?
			// I suspect it would be nice to convert these to sane formats based on profiles.
			// something for later..
			dstImgFmt = src.getFormat();
		}


		if (Util::isDxt(src.getFormat())) 
		{
			if (dstImgFmt != src.getFormat())
			{
				X_WARNING("Img", "Source image is already: %s compressed, skipping conversion to: %s",
					Texturefmt::ToString(src.getFormat()), Texturefmt::ToString(dstImgFmt));

				dstImgFmt = src.getFormat();
			}
			else
			{
				// warn about provided block formats as raw, but it's currently our target fmt for this img, so proceed.
				X_WARNING("Img", "Source image is already: %s compressed",
					Texturefmt::ToString(src.getFormat()));
			}
		}

		if (!flags.IsSet(CompileFlag::ALPHA))
		{
			// auto set alpha?
			switch (src.getFormat())
			{
			case Texturefmt::R8G8B8A8:
			case Texturefmt::R8G8B8A8_SRGB:
			case Texturefmt::R8G8B8A8_SNORM:
			case Texturefmt::R8G8B8A8_TYPELESS:
			case Texturefmt::R8G8B8A8_SINT:
			case Texturefmt::R8G8B8A8_UINT:
			case Texturefmt::A8R8G8B8:
			// humm
			case Texturefmt::B8G8R8A8:
			case Texturefmt::B8G8R8A8_SRGB:
			case Texturefmt::B8G8R8A8_TYPELESS:

			// these have alpha channels.
			// not gonna support BC1 1 bit alpha for now.
			case Texturefmt::BC3:
			case Texturefmt::BC3_SRGB:
			case Texturefmt::BC3_TYPELESS:
			case Texturefmt::BC7:
			case Texturefmt::BC7_SRGB:
			case Texturefmt::BC7_TYPELESS:

				X_LOG2("Img", "Setting alpha flag due to import format: %s", Texturefmt::ToString(src.getFormat()));
				flags.Set(CompileFlag::ALPHA);
				break;
			}
		}	

		// we need alpha channel
		if (!Util::hasAlpha(src.getFormat()))
		{
			if (!con.addAlphachannel(flags.IsSet(CompileFlag::IGNORE_SRC_MIPS))) {
				X_WARNING("Img", "Failed to add alpha channel to src fmt: \"%s\"", Texturefmt::ToString(src.getFormat()));
				return false;
			}
		}
	}

	// now what :(
	// the main things that is going to be a pain is taking N formats and allow converting to N formats.
	// realistically that is going to be a pain.
	// so i need some sort of conversion groups that define what formats can be come what.
	// 
	// we don't express these groups based on file formats, but format of loaded data.
	// the file format the data came from is not important.
	//
	// if the format is already correct we can just write to CI.
	// but it may need mip map generation which i will only support for some formats.
	// what are the logical steps tho?
	//
	// - LoadImg
	//		if we do a lot of scaling, adding support for mip skip might be nice
	// - Create Mips
	//		skip if noMips
	// - Convert each mip
	//		skipping high mips if scaled
	//		
	// - Write data?
	// - flex pecs? (required)
	// 
	//  Where do we perform premultiply alpha?
	// 

	if (!flags.IsSet(CompileFlag::NOMIPS)) {

		MipMapFilterParams filterParams;
		ImgConveter::getDefaultFilterWidthAndParams(mipFilter, filterParams);

		core::StopWatch timer;

		if (!con.CreateMips(mipFilter, filterParams, wrapMode, flags.IsSet(CompileFlag::ALPHA), flags.IsSet(CompileFlag::IGNORE_SRC_MIPS))) {
			X_ERROR("Img", "Failed to create mips for image");
			return false;
		}

		X_LOG1("Img", "Mipmap creation took: ^6%g ms", timer.GetMilliSeconds());
	}
	else {
		// this weould be kind easy to support tbh.
		// just generate the mips 
		// drop the top mips
		// and trim the lower mips after.
		if (scale != ScaleFactor::ORIGINAL) {
			X_ERROR("Img", "Scaling is not currently supported when mips are disbled");
			return false;
		}
	}

	con.scale(scale);

	{
		core::StopWatch timer;

		if (!con.Convert(dstImgFmt, qualityProfile, flags.IsSet(CompileFlag::ALPHA))) {
			X_ERROR("Img", "Failed to convert image");
			return false;
		}

		X_LOG1("Img", "Conversion took: ^6%g ms", timer.GetMilliSeconds());
	}

	if (!con.SaveImg(destPath, flags, outputFileFmt)) {
		X_ERROR("Img", "Failed to save converterd image");
		return false;
	}

	// all good in the hood BOI!
	return true;
}



X_NAMESPACE_END