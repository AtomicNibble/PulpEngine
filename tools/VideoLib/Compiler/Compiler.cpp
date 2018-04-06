#include "stdafx.h"
#include "Compiler.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(video)

VideoCompiler::VideoCompiler(core::MemoryArenaBase* arena) :
    arena_(arena),
    videoData_(arena)
{
}

VideoCompiler::~VideoCompiler()
{
}

bool VideoCompiler::setVideo(DataVec&& videoData)
{
    videoData_ = std::move(videoData);
    return true;
}

bool VideoCompiler::writeToFile(core::XFile* pFile) const
{
    if (pFile->write(videoData_.data(), videoData_.size()) != videoData_.size()) {
        X_ERROR("Video", "Failed to write data");
        return false;
    }

    return true;
}

X_NAMESPACE_END