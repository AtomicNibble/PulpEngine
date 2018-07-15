#include "stdafx.h"
#include "FontLib.h"

#include <IFileSys.h>

#include "Compiler\Compiler.h"

X_NAMESPACE_BEGIN(font)

FontLib::FontLib()
{
}

FontLib::~FontLib()
{
}

const char* FontLib::getOutExtension(void) const
{
    return FONT_BAKED_FILE_EXTENSION;
}

bool FontLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    X_UNUSED(host, assetId, args, destPath);

    // load trueTypeFont from assetDB.
    core::Array<uint8_t> fileData(host.getScratchArena());
    if (!host.GetAssetData(assetId, fileData)) {
        X_ERROR("Font", "Failed to get asset data");
        return false;
    }

    if (fileData.isEmpty()) {
        X_ERROR("Font", "File data is empty");
        return false;
    }

    int32_t width = 32;
    int32_t height = 32;
    float sizeRatio = 1.f;
    bool sdf = true;

    {
        core::json::Document d;
        if (d.Parse(args.c_str(), args.length()).HasParseError()) {
            X_ERROR("Font", "Error parsing json");
            return false;
        }

        std::array<std::pair<const char*, core::json::Type>, 3> requiredValues = {{{"glyphWidth", core::json::kNumberType},
            {"glyphHeight", core::json::kNumberType},
            {"sizeRatio", core::json::kNumberType}}};

        for (size_t i = 0; i < requiredValues.size(); i++) {
            const auto& item = requiredValues[i];
            if (!d.HasMember(item.first)) {
                X_ERROR("Font", "Missing required value: \"%s\"", item.first);
                return false;
            }

            if (d[item.first].GetType() != item.second) {
                X_ERROR("Font", "Incorrect type for \"%s\"", item.first);
                return false;
            }
        }

        width = d["glyphWidth"].GetInt();
        height = d["glyphHeight"].GetInt();
        sizeRatio = d["sizeRatio"].GetFloat();
    }

    FontCompiler compiler(g_FontLibArena);
    if (!compiler.setFont(std::move(fileData), width, height, sizeRatio)) {
        X_ERROR("Font", "Failed to set source font data");
        return false;
    }

    if (!compiler.loadFromJson(args)) {
        X_ERROR("Font", "Failed to parse font desc");
        return false;
    }

    if (!compiler.bake(sdf)) {
        X_ERROR("Font", "Failed to bake font data");
        return false;
    }

    core::XFileScoped file;
    core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

    if (!file.openFile(destPath.c_str(), mode)) {
        X_ERROR("Font", "Failed to open output file");
        return false;
    }

    if (!compiler.writeToFile(file.GetFile())) {
        X_ERROR("Font", "Failed to write baked font file");
        return false;
    }

    file.close();

    // dump a image of the glyphs.
#if 0
		OutPath imagePath(destPath);
		imagePath.append(".bmp");
		if (!file.openFile(imagePath.c_str(), mode)) {
			X_ERROR("Font", "Failed to open img file");
			return false;
		}

		if (!compiler.writeImageToFile(file.GetFile())) {
			X_ERROR("Font", "Failed to write baked font file");
			return false;
		}
#endif

    return true;
}

X_NAMESPACE_END