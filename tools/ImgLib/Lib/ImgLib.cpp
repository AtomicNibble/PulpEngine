#include "stdafx.h"
#include "ImgLib.h"


#include <String\StringHash.h>


X_NAMESPACE_BEGIN(texture)


ImgLib::ImgLib()
{

}

ImgLib::~ImgLib()
{

}

namespace
{
	struct FmtStr
	{
		core::StrHash nameHash;
		const char* pName;
		Texturefmt::Enum fmt;
	};

	FmtStr formats[] = 
	{
		{ core::StrHash("A8"), "A8", Texturefmt::A8 },
		{ core::StrHash("R8G8"), "R8G8", Texturefmt::R8G8 },
		{ core::StrHash("R8G8_TYPELESS"), "R8G8_TYPELESS", Texturefmt::R8G8_TYPELESS },
		{ core::StrHash("R8G8_SNORM"), "R8G8_SNORM", Texturefmt::R8G8_SNORM },
		{ core::StrHash("R8G8_UNIT"), "R8G8_UNIT", Texturefmt::R8G8_UNIT },
		{ core::StrHash("R8G8_SINT"), "R8G8_SINT", Texturefmt::R8G8_SINT },
		{ core::StrHash("R16G16_FLOAT"), "R16G16_FLOAT", Texturefmt::R16G16_FLOAT },
		{ core::StrHash("R16G16"), "R16G16", Texturefmt::R16G16 },
		{ core::StrHash("R16G16_SRGB"), "R16G16_SRGB", Texturefmt::R16G16_SRGB },
		{ core::StrHash("R16G16_SNORM"), "R16G16_SNORM", Texturefmt::R16G16_SNORM },
		{ core::StrHash("R16G16_SINT"), "R16G16_SINT", Texturefmt::R16G16_SINT },
		{ core::StrHash("R16G16_UINT"), "R16G16_UINT", Texturefmt::R16G16_UINT },
		{ core::StrHash("R16G16_TYPELESS"), "R16G16_TYPELESS", Texturefmt::R16G16_TYPELESS },
		{ core::StrHash("R8G8B8"), "R8G8B8", Texturefmt::R8G8B8 },
		{ core::StrHash("B8G8R8"), "B8G8R8", Texturefmt::B8G8R8 },
		{ core::StrHash("R8G8B8A8"), "R8G8B8A8", Texturefmt::R8G8B8A8 },
		{ core::StrHash("R8G8B8A8_SRGB"), "R8G8B8A8_SRGB", Texturefmt::R8G8B8A8_SRGB },
		{ core::StrHash("R8G8B8A8_SNORM"), "R8G8B8A8_SNORM", Texturefmt::R8G8B8A8_SNORM },
		{ core::StrHash("R8G8B8A8_TYPELESS"), "R8G8B8A8_TYPELESS", Texturefmt::R8G8B8A8_TYPELESS },
		{ core::StrHash("R8G8B8A8_TYPELESS"), "R8G8B8A8_TYPELESS", Texturefmt::R8G8B8A8_TYPELESS },
		{ core::StrHash("R8G8B8A8_SINT"), "R8G8B8A8_SINT", Texturefmt::R8G8B8A8_SINT },
		{ core::StrHash("R8G8B8A8_UINT"), "R8G8B8A8_UINT", Texturefmt::R8G8B8A8_UINT },
		{ core::StrHash("A8R8G8B8"), "A8R8G8B8", Texturefmt::A8R8G8B8 },
		{ core::StrHash("B8G8R8A8"), "B8G8R8A8", Texturefmt::B8G8R8A8 },
		{ core::StrHash("B8G8R8A8_SRGB"), "B8G8R8A8_SRGB", Texturefmt::B8G8R8A8_SRGB },
		{ core::StrHash("B8G8R8A8_TYPELESS"), "B8G8R8A8_TYPELESS", Texturefmt::B8G8R8A8_TYPELESS },
		{ core::StrHash("ATI2"), "ATI2", Texturefmt::ATI2 },
		{ core::StrHash("ATI2_XY"), "ATI2_XY", Texturefmt::ATI2_XY },
		{ core::StrHash("BC1"), "BC1", Texturefmt::BC1 },
		{ core::StrHash("BC1_SRGB"), "BC1_SRGB", Texturefmt::BC1_SRGB },
		{ core::StrHash("BC1_TYPELESS"), "BC1_TYPELESS", Texturefmt::BC1_TYPELESS },
		{ core::StrHash("BC2"), "BC2", Texturefmt::BC2 },
		{ core::StrHash("BC2_SRGB"), "BC2_SRGB", Texturefmt::BC2_SRGB },
		{ core::StrHash("BC2_TYPELESS"), "BC2_TYPELESS", Texturefmt::BC2_TYPELESS },
		{ core::StrHash("BC3"), "BC3", Texturefmt::BC3 },
		{ core::StrHash("BC3_SRGB"), "BC3_SRGB", Texturefmt::BC3_SRGB },
		{ core::StrHash("BC3_TYPELESS"), "BC3_TYPELESS", Texturefmt::BC3_TYPELESS },
		{ core::StrHash("BC4"), "BC4", Texturefmt::BC4 },
		{ core::StrHash("BC4_SNORM"), "BC4_SNORM", Texturefmt::BC4_SNORM },
		{ core::StrHash("BC4_TYPELESS"), "BC4_TYPELESS", Texturefmt::BC4_TYPELESS },
		{ core::StrHash("BC5"), "BC5", Texturefmt::BC5 },
		{ core::StrHash("BC5_SNORM"), "BC5_SNORM", Texturefmt::BC5_SNORM },
		{ core::StrHash("BC5_TYPELESS"), "BC5_TYPELESS", Texturefmt::BC5_TYPELESS },
		{ core::StrHash("BC6"), "BC6", Texturefmt::BC6 },
		{ core::StrHash("BC6_SF16"), "BC6_SF16", Texturefmt::BC6_SF16 },
		{ core::StrHash("BC6_TYPELESS"), "BC6_TYPELESS", Texturefmt::BC6_TYPELESS },
		{ core::StrHash("BC7"), "BC7", Texturefmt::BC7 },
		{ core::StrHash("BC7_SRGB"), "BC7_SRGB", Texturefmt::BC7_SRGB },
		{ core::StrHash("BC7_TYPELESS"), "BC7_TYPELESS", Texturefmt::BC7_TYPELESS },
		{ core::StrHash("R10G10B10A2"), "R10G10B10A2", Texturefmt::R10G10B10A2 },
	};


	bool ParseImgFmt(const char* pImgFmt, Texturefmt::Enum& fmtOut)
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



} // namespace 


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
	ImgFileFormat::Enum outputFileFmt = ImgFileFormat::UNKNOWN;
	MipFilter::Enum mipFilter = MipFilter::DEFAULT;
	WrapMode::Enum wrapMode = WrapMode::Mirror;
	Texturefmt::Enum imgFmt = Texturefmt::UNKNOWN;

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
			flags.Set(CompileFlag::NO_MIPS);
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

		if (!ParseImgFmt(pImgFmt, imgFmt)) {
			X_ERROR("Img", "Unknown img fmt: \"%s\"", pImgFmt);
			return false;
		}

		// now we check it's one we support converting as well as loading :(
		switch (imgFmt)
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
	X_ASSERT(inputFileFmt != ImgFileFormat::UNKNOWN, "inputFile Fmt is invalid")(inputFileFmt);
	X_ASSERT(outputFileFmt != ImgFileFormat::UNKNOWN, "OutputFile Fmt is invalid")(outputFileFmt);






	return false;
}

X_NAMESPACE_END