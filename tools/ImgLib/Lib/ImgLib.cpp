#include "stdafx.h"
#include "ImgLib.h"


#include <String\StringHash.h>
#include "Converter\Converter.h"

X_NAMESPACE_BEGIN(texture)

using namespace Converter;

ImgLib::ImgLib()
{

}

ImgLib::~ImgLib()
{

}


bool ImgLib::Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
	const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	core::json::Document d;
	d.Parse(args.c_str());


	if (fileData.isEmpty()) {
		X_ERROR("ImgLib", "File data is empty");
		return false;
	}

	CompileFlags flags;
	ScaleFactor::Enum scale = ScaleFactor::ORIGINAL;
	ImgFileFormat::Enum inputFileFmt = ImgFileFormat::UNKNOWN;
	ImgFileFormat::Enum outputFileFmt = ImgFileFormat::CI;
	MipFilter::Enum mipFilter = MipFilter::Box;
	WrapMode::Enum wrapMode = WrapMode::Mirror;
	Texturefmt::Enum dstImgFmt = Texturefmt::UNKNOWN;

	// Required args:
	// * imgPath
	// * imgFmt (for now)
	// 

	if (d.HasMember("imgPath")) {
		const char* pImgPath = d["imgPath"].GetString();
		const char* pFileExt = core::strUtil::FileExtension(pImgPath);

		// currently i think best way to detect img format is based on ext.
		// should handle all cases fine, other than some retard saving shit without extensions.
		// are you that retard?
		if (!pFileExt) {
			X_ERROR("ImgLib", "Input file missing file extension: \"%s\"", pImgPath);
			return false;
		}

		if (core::strUtil::IsEqualCaseInsen(pFileExt, "dds")) {
			inputFileFmt = ImgFileFormat::DDS;
		} else if (core::strUtil::IsEqualCaseInsen(pFileExt, "png")) {
			inputFileFmt = ImgFileFormat::PNG;
		} else if (core::strUtil::IsEqualCaseInsen(pFileExt, "jpg")) {
			inputFileFmt = ImgFileFormat::JPG;
		} else if (core::strUtil::IsEqualCaseInsen(pFileExt, "psd")) {
			inputFileFmt = ImgFileFormat::PSD;
		} else if (core::strUtil::IsEqualCaseInsen(pFileExt, "tga")) {
			inputFileFmt = ImgFileFormat::TGA;
		} else if (core::strUtil::IsEqualCaseInsen(pFileExt, "ci")) {
			// you can't convert from a CI it's not allowed!
			X_ERROR("ImgLib", "Input format of type CI is not allowed");
			return false;
		}
		else {
			X_ERROR("ImgLib", "Unkown input image extension: \"%s\"", pFileExt);
			return false;
		}
	}
	else {
		X_ERROR("ImgLib", "Missing requited argument: imgPath");
		return false;
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
	if(d.HasMember("imgFmt"))
	{
		const char* pImgFmt = d["imgFmt"].GetString();

		if (!ImgConveter::ParseImgFmt(pImgFmt, dstImgFmt)) {
			X_ERROR("Img", "Unknown img fmt: \"%s\"", pImgFmt);
			return false;
		}

		// now we check it's one we support converting as well as loading :(
		switch (dstImgFmt)
		{
		case Texturefmt::BC1:
		case Texturefmt::BC3:
		case Texturefmt::BC6:
		case Texturefmt::BC7:
			break;

		default:
			X_ERROR("Img", "Format not implemented for conversion: \"%s\"", pImgFmt);
			return false;
		}
	}
	else
	{
		// for certain types of images we could auto select..
		// but we don't currently.
		X_ASSERT_NOT_IMPLEMENTED();
		return false;
	}


	// check we only got here with valid shit!
	X_ASSERT(inputFileFmt != ImgFileFormat::UNKNOWN, "InputFile Fmt is invalid")(inputFileFmt);
	X_ASSERT(outputFileFmt != ImgFileFormat::UNKNOWN, "OutputFile Fmt is invalid")(outputFileFmt);


	ImgConveter con(g_ImgLibArena, g_ImgLibArena);

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

		if (!con.CreateMips(mipFilter, filterParams, wrapMode, flags.IsSet(CompileFlag::ALPHA), flags.IsSet(CompileFlag::IGNORE_SRC_MIPS))) {
			X_ERROR("Img", "Failed to create mips for image");
			return false;
		}
	}

	if (!con.Convert(dstImgFmt)) {
		X_ERROR("Img", "Failed to convert image");
		return false;
	}

	if (!con.SaveImg(destPath, flags, outputFileFmt)) {
		X_ERROR("Img", "Failed to save converterd image");
		return false;
	}

	// all good in the hood BOI!
	return true;
}



X_NAMESPACE_END