#include "stdafx.h"
#include "AssertHandler.h"

X_NAMESPACE_BEGIN(physics)

AssetHandler gAssetHandler;

void AssetHandler::operator()(const char* exp, const char* file, int line, bool& ignore)
{
    X_UNUSED(ignore);

#if X_ENABLE_ASSERTIONS
    core::SourceInfo sourceInfo(file, line, "", "");
    core::Assert(sourceInfo, "Assertion \"%s\" failed.", exp)
        .Variable("FileName", file)
        .Variable("LineNumber", line);

#else
    X_ERROR("SoundSys", "Sound system threw a assert: Exp: \"%s\" file: \"%s\" line: \"%s\"",
        exp, file, line);
#endif

    X_BREAKPOINT;
}

X_NAMESPACE_END