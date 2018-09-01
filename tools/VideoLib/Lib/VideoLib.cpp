#include "stdafx.h"
#include "VideoLib.h"

#include <IFileSys.h>

#include "Compiler\Compiler.h"

X_NAMESPACE_BEGIN(video)

VideoLib::VideoLib()
{
}

VideoLib::~VideoLib()
{
}

const char* VideoLib::getOutExtension(void) const
{
    return VIDEO_FILE_EXTENSION;
}

bool VideoLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    X_UNUSED(args);

    core::Array<uint8_t> fileData(host.getScratchArena());
    if (!host.GetAssetData(assetId, fileData)) {
        X_ERROR("Video", "Failed to get asset data");
        return false;
    }

    if (fileData.isEmpty()) {
        X_ERROR("Video", "File data is empty");
        return false;
    }

    VideoCompiler compiler(g_VideoLibArena);
    if (!compiler.process(std::move(fileData))) {
        X_ERROR("Video", "Failed to set source video data");
        return false;
    }

    core::XFileScoped file;
    core::fileModeFlags mode = core::fileMode::RECREATE | core::fileMode::WRITE;

    if (!file.openFile(destPath.c_str(), mode)) {
        X_ERROR("Video", "Failed to open output file");
        return false;
    }

    if (!compiler.writeToFile(file.GetFile())) {
        X_ERROR("Video", "Failed to write video file");
        return false;
    }

    return true;
}

X_NAMESPACE_END
