#include "stdafx.h"
#include "ImgLib.h"

#include <Time\StopWatch.h>

#include <String\StringHash.h>
#include "Converter\Converter.h"
#include "Util\FloatImage.h"
#include "Util\Filters.h"

#include <ICi.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(texture)

using namespace Converter;

namespace
{
    X_DECLARE_ENUM(CompressionMethod)(
        CompressHighCol,
        CompressHighAlpha,
        CompressCol,
        NoCompress,
        Custom
    );

} // namespace

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

bool ImgLib::thumbGenerationSupported(void) const
{
    return true;
}

bool ImgLib::repackSupported(void) const
{
    return true;
}

bool ImgLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    X_UNUSED(host);
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pJobSys);

    core::json::Document d;
    d.Parse(args.c_str(), args.length());

    CompileFlags flags;
    ScaleFactor::Enum scale = ScaleFactor::ORIGINAL;
    ImgFileFormat::Enum outputFileFmt = ImgFileFormat::CI;
    MipFilter::Enum mipFilter = MipFilter::Box;
    WrapMode::Enum wrapMode = WrapMode::Clamp;
    Texturefmt::Enum dstImgFmt = Texturefmt::UNKNOWN;

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
        }
        else if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/2")) {
            scale = ScaleFactor::HALF;
        }
        else if (core::strUtil::IsEqualCaseInsen(pScaleFactor, "1/4")) {
            scale = ScaleFactor::FOURTH;
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

        if (core::strUtil::IsEqualCaseInsen(pOutFmt, "ci")) {
            outputFileFmt = ImgFileFormat::CI;
        }
        else if (!core::strUtil::IsEqualCaseInsen(pOutFmt, "dds")) {
            outputFileFmt = ImgFileFormat::DDS;
        }
        else {
            X_ERROR("Img", "Ouput file fmt currently not supported: \"%s\" using ci", pOutFmt);
            outputFileFmt = ImgFileFormat::CI;
        }
    }
    if (d.HasMember("wrap")) {
        const char* pWrap = d["wrap"].GetString();

        if (!core::strUtil::IsEqualCaseInsen(pWrap, "Mirror")) {
            wrapMode = WrapMode::Mirror;
        }
        else if (!core::strUtil::IsEqualCaseInsen(pWrap, "Repeat")) {
            wrapMode = WrapMode::Repeat;
        }
        else if (!core::strUtil::IsEqualCaseInsen(pWrap, "Clamp")) {
            wrapMode = WrapMode::Clamp;
        }
        else {
            X_ERROR("Img", "Wrap mode not supported: \"%s\"", pWrap);
            return false;
        }
    }

    // we want to support converting to all the supported image formats.
    // which is basically the Texturefmt enum.
    // which I really can't be botherd to fully support for conversion.
    CompressionMethod::Enum compMethod;

    if (d.HasMember("compressionMethod")) {
        const auto& val = d["compressionMethod"];
        using namespace core::Hash::Literals;

        /*
			Best color compression[compressHighCol]
			Better alpha compression[compressHighAlpha]
			Low quality color compression with no Alpha[compressCol]
			Uncompressed[noCompress]
			Custom[custom]
		*/

        switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength())) {
            case "compressHighCol"_fnv1a:
                compMethod = CompressionMethod::CompressHighCol;
                break;
            case "compressHighAlpha"_fnv1a:
                compMethod = CompressionMethod::CompressHighAlpha;
                break;
            case "compressCol"_fnv1a:
                compMethod = CompressionMethod::CompressCol;
                break;
            case "noCompress"_fnv1a:
                compMethod = CompressionMethod::NoCompress;
                break;
            case "custom"_fnv1a:
                compMethod = CompressionMethod::Custom;
                break;

            default:
                X_ERROR("Img", "Unknown compressionMethod: \"%.*s\"", val.GetStringLength(), val.GetString());
                return false;
        }
    }

    if (compMethod == CompressionMethod::Custom) {
        if (!d.HasMember("imgFmt")) {
            X_ERROR("Img", "imgFmt option required for custom compressionMethod");
            return false;
        }

        const char* pImgFmt = d["imgFmt"].GetString();

        dstImgFmt = Util::TexFmtFromStr(pImgFmt);
        if (dstImgFmt == Texturefmt::UNKNOWN) {
            X_ERROR("Img", "Unknown img fmt: \"%s\"", pImgFmt);
            return false;
        }
    }

    // check we only got here with valid shit!
    X_ASSERT(outputFileFmt != ImgFileFormat::UNKNOWN, "OutputFile Fmt is invalid")(outputFileFmt); 

    ImgConveter::Profile::Enum qualityProfile = ImgConveter::Profile::VeryFast;
    ImgConveter con(g_ImgLibArena, g_ImgLibArena);

    // we support global overrides.
    core::string conProfile; // this is COW, so cheap to get a copy here.
    if (host.getConversionProfileData(assetDb::AssetType::IMG, conProfile)) {
        core::json::Document pd;
        pd.Parse(conProfile.c_str(), conProfile.length());

        for (auto it = pd.MemberBegin(); it != pd.MemberEnd(); ++it) {
            const auto& name = it->name;
            const auto& val = it->value;

            using namespace core::Hash::Literals;

            switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength())) {
                case "outFmt"_fnv1a:
                    switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength())) {
                        case "ci"_fnv1a:
                            outputFileFmt = ImgFileFormat::CI;
                            break;
                        case "dds"_fnv1a:
                            outputFileFmt = ImgFileFormat::DDS;
                            break;
                        default:
                            X_WARNING("Img", "Unknown outFmt: %.*s", val.GetStringLength(), val.GetString());
                            break;
                    }

                    break;
                case "qualityProfile"_fnv1a:
                    switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength())) {
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
                    if (val.GetType() == core::json::Type::kNumberType) {
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
                    if (val.GetType() == core::json::Type::kNumberType) {
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
    if (!con.loadImg(fileData, inputFileFmt)) {
        X_ERROR("Img", "Failed to load source image");
        return false;
    }

    {
        const auto& src = con.getTextFile();
        X_ASSERT(src.isValid(), "Src img not valid after successful loading")(src.isValid()); 

        // pick a format.
        switch (compMethod) {
            case CompressionMethod::CompressHighCol:
                if (src.getFormat() == Texturefmt::A8) {
                    dstImgFmt = Texturefmt::A8;
                }
                else {
                    dstImgFmt = Texturefmt::BC7;
                }
                break;
            case CompressionMethod::CompressHighAlpha:
                dstImgFmt = Texturefmt::BC3;
                break;
            case CompressionMethod::CompressCol:
                dstImgFmt = Texturefmt::BC1;
                break;
            case CompressionMethod::NoCompress:
                if (src.getFormat() == Texturefmt::A8) {
                    dstImgFmt = Texturefmt::A8;
                }
                else {
                    dstImgFmt = Texturefmt::R8G8B8A8;
                }
                break;
            case CompressionMethod::Custom:
                X_ASSERT(dstImgFmt != Texturefmt::UNKNOWN, "Texture format not set for custom")();
                break;

            default:
                X_ASSERT_NOT_IMPLEMENTED();
                return false;
        }


        // now we check i support creating the target format.
        switch (dstImgFmt) {
            case Texturefmt::A8:
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

        if (!core::bitUtil::IsPowerOfTwo(src.getWidth()) || !core::bitUtil::IsPowerOfTwo(src.getHeight())) {
            // none pow 2 img :| https://winpic.co/MAbde113843b.gif

            if (!flags.IsSet(CompileFlag::ALLOW_NONE_POW2)) {
                X_ERROR("Img", "Image is not power of two: %" PRIi32 "x%" PRIi32, src.getWidth(), src.getHeight());
                return false;
            }

            // when allowing none pow2 images, we need to make sure block formats are processed correct.
            // throw this here for now.
            X_ASSERT_NOT_IMPLEMENTED();
            return false;
        }

        if (flags.IsSet(CompileFlag::NO_COMPRESSION)) {
            // you want none compressed but gave me a block format, fuck you!
            if (Util::isDxt(src.getFormat())) {
                X_ERROR("Img", "No compression requested, but source format(%s) is already block compressed. bitch please..",
                    Texturefmt::ToString(src.getFormat()));
                return false;
            }

            // just leave as src format?
            // I suspect it would be nice to convert these to sane formats based on profiles.
            // something for later..
            dstImgFmt = src.getFormat();
        }

        if (Util::isDxt(src.getFormat())) {
            if (dstImgFmt != src.getFormat()) {
                X_WARNING("Img", "Source image is already: %s compressed, skipping conversion to: %s",
                    Texturefmt::ToString(src.getFormat()), Texturefmt::ToString(dstImgFmt));

                dstImgFmt = src.getFormat();
            }
            else {
                // warn about provided block formats as raw, but it's currently our target fmt for this img, so proceed.
                X_WARNING("Img", "Source image is already: %s compressed",
                    Texturefmt::ToString(src.getFormat()));
            }
        }

        if (!flags.IsSet(CompileFlag::ALPHA)) {
            static_assert(Texturefmt::ENUM_COUNT == 56, "Added additional texture fmts? this code needs updating.");

            // auto set alpha?
            switch (src.getFormat()) {
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

                default:
                    break;
            }
        }

        if (src.getFormat() != Texturefmt::A8) { // TODO: we only need alpha channel if target is a dxt format.
            // we need alpha channel
            if (!Util::hasAlpha(src.getFormat())) {
                if (!con.addAlphachannel(flags.IsSet(CompileFlag::IGNORE_SRC_MIPS))) {
                    X_WARNING("Img", "Failed to add alpha channel to src fmt: \"%s\"", Texturefmt::ToString(src.getFormat()));
                    return false;
                }
            }
        }
    }

    if (!flags.IsSet(CompileFlag::NOMIPS)) {
        MipMapFilterParams filterParams;
        ImgConveter::getDefaultFilterWidthAndParams(mipFilter, filterParams);

        core::StopWatch timer;

        if (!con.createMips(mipFilter, filterParams, wrapMode, flags.IsSet(CompileFlag::ALPHA), flags.IsSet(CompileFlag::IGNORE_SRC_MIPS))) {
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

        if (!con.convert(dstImgFmt, qualityProfile, flags.IsSet(CompileFlag::ALPHA))) {
            X_ERROR("Img", "Failed to convert image");
            return false;
        }

        X_LOG1("Img", "Conversion took: ^6%g ms", timer.GetMilliSeconds());
    }

    // set the path to format.
    // this breaks the converters is stale check.
    // but for the converter we always want ci for the game, dds is just for testing.
    auto path(destPath);
    path.setExtension(Util::getExtension(outputFileFmt));

    if (!con.saveImg(path, flags, outputFileFmt)) {
        X_ERROR("Img", "Failed to save converterd image");
        return false;
    }

    // all good in the hood BOI!
    return true;
}

bool ImgLib::CreateThumb(IConverterHost& host, int32_t assetId, Vec2i targetDim)
{
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

    ImgFileFormat::Enum inputFileFmt = Util::resolveSrcfmt(fileData);
    if (inputFileFmt == ImgFileFormat::UNKNOWN) {
        X_ERROR("Img", "Unknown img src format");
        return false;
    }

    // load it :D
    XTextureFile srcImg(host.getScratchArena());
    if (!Util::loadImage(host.getScratchArena(), fileData, inputFileFmt, srcImg)) {
        X_WARNING("Img", "Failed to load src image");
        return false;
    }

    if (srcImg.getDepth() > 1 || srcImg.getNumFaces() > 1) {
        X_WARNING("Img", "Thumb generation for multi face images not currenlty supported");
        return false;
    }

    Vec2i srcDim(srcImg.getWidth(), srcImg.getHeight());

    // remove mips
    srcImg.dropMips();

    // we want to shrink it down.
    FloatImage fltImg(host.getScratchArena());
    FloatImage fltThumb(host.getScratchArena());
    if (!fltImg.initFrom(srcImg, 0, 0)) {
        X_ERROR("Img", "Failed to load src image into FltImage");
        return false;
    }

    BoxFilter filter;
    fltImg.resize(fltThumb, host.getScratchArena(), filter, targetDim.x, targetDim.y, WrapMode::Clamp);

    // now we want a textureFile.
    XTextureFile thumb(host.getScratchArena());
    thumb.setDepth(1);
    thumb.setNumFaces(1);
    thumb.setNumMips(1);
    thumb.setWidth(targetDim.x);
    thumb.setHeigth(targetDim.y);
    thumb.setType(TextureType::T2D);
    if (Util::hasAlpha(srcImg.getFormat())) {
        thumb.setFormat(Texturefmt::R8G8B8A8);
    }
    else {
        thumb.setFormat(Texturefmt::R8G8B8);
    }
    thumb.resize();

    // save the float image to it.
    if (!fltThumb.saveToImg(thumb, 0, 0)) {
        X_ERROR("Img", "Failed to save thumb image");
        return false;
    }

    // now save this image as PNG to a memory buffer.
    core::XFileStream file(host.getScratchArena());
    if (!Util::saveImage(host.getScratchArena(), &file, ImgFileFormat::PNG, thumb)) {
        X_ERROR("Img", "Failed to save thumb image");
        return false;
    }

    // sanity.
    const auto buf = file.buffer();
    if (buf.isEmpty()) {
        X_ERROR("Img", "Thumb data is empty");
        return false;
    }

    // for png i should probs just use 'store'.
    // or maybe save as rgb8 raw in tga or dds and lz4 high it.
    // all depends on what i plan to view these thumbs in supports / likes.
    host.UpdateAssetThumb(assetId, targetDim, srcDim, buf, core::Compression::Algo::STORE, core::Compression::CompressLevel::NORMAL);
    return true;
}

bool ImgLib::Repack(IConverterHost& host, assetDb::AssetId assetId) const
{
    // HELLO !
    // would you like me to repack a pickle?
    // now that is a mighty request.

    // so basically I want to load the current raw data see what image format it is.
    // convert it and compress it if deemed required.
    core::Array<uint8_t> fileData(host.getScratchArena());
    if (!host.GetAssetData(assetId, fileData)) {
        X_ERROR("Img", "Failed to get asset data");
        return false;
    }

    if (fileData.isEmpty()) {
        X_ERROR("Img", "File data is empty");
        return false;
    }

    ImgFileFormat::Enum inputFileFmt = Util::resolveSrcfmt(fileData);
    if (inputFileFmt == ImgFileFormat::UNKNOWN) {
        X_ERROR("Img", "Unknown img src format");
        return false;
    }

    core::Compression::Algo::Enum compAlgo;
    if (!host.GetAssetDataCompAlgo(assetId, compAlgo)) {
        X_ERROR("Img", "Failed to get comp algo");
        return false;
    }

    auto targetAlgo = core::Compression::Algo::LZ4HC;

    // we kinda need to know the algo.
    switch (inputFileFmt)
    {
        // if it's dds just make sure compressed?
        case ImgFileFormat::CI:
        case ImgFileFormat::DDS:
        case ImgFileFormat::TGA:
        case ImgFileFormat::PSD:
            // re compress.
            break;

        case ImgFileFormat::JPG:
            // just store JGP no real gains to be had.
            // or can convert to TGA and compress, but don't see point.
            targetAlgo = core::Compression::Algo::STORE;
            break;

        case ImgFileFormat::PNG:
        {
            // turn to uncompressed format.
            XTextureFile srcImg(host.getScratchArena());
            if (!Util::loadImage(host.getScratchArena(), fileData, inputFileFmt, srcImg)) {
                X_WARNING("Img", "Failed to load src image");
                return false;
            }

            if (srcImg.getFormat() == Texturefmt::R8G8B8) {
                targetAlgo = core::Compression::Algo::STORE;
                break;
            }

            X_LOG0("Img", "Repacking png");

            core::XFileStream fileStream(host.getScratchArena());

            if (!Util::saveImage(host.getScratchArena(), &fileStream, ImgFileFormat::DDS, srcImg)) {
                X_ERROR("Img", "Failed to save image");
                return false;
            }

            // TODO: make array<T> have a base type so no matter grow policy can assign, simular to byteStream.
            auto& buffer = fileStream.buffer();

            fileData.resize(buffer.size());
            std::memcpy(fileData.data(), buffer.data(), fileData.size());
            break;
        }

        default:
            break;
    }

    // send the raw data back to asset db with desired algo.
    if (!host.UpdateAssetRawFile(assetId, fileData, targetAlgo, core::Compression::CompressLevel::HIGH)) {
        return false;
    }

    return true;
}


X_NAMESPACE_END