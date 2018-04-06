#include <EngineCommon.h>

#include "CallStack.h"

#include <Debugging\SymbolInfo.h>
#include <Debugging\SymbolResolution.h>

X_NAMESPACE_BEGIN(core)

CallStack::CallStack(unsigned int numFramesToSkip)
{
    core::zero_object(frames_);

    RtlCaptureStackBackTrace(numFramesToSkip + 1, MAX_FRAMES, frames_, 0);
}

const char* CallStack::ToDescription(Description& description) const
{
    return CallStack::ToDescription("", description);
}

const char* CallStack::ToDescription(const char* info, Description& description) const
{
    const void* address;
    int offset;

    offset = _snprintf_s(description, sizeof(Description), _TRUNCATE, "%s", info);

    for (int i = 0; i < MAX_FRAMES; ++i) {
        address = frames_[i];
        if (!address)
            break;

        SymbolInfo result = symbolResolution::ResolveSymbolsForAddress(address);

        offset += _snprintf_s(&description[offset], sizeof(Description) - offset, _TRUNCATE, "%s(%d): %s (0x%08p)\n",
            core::strUtil::FileName(result.GetFilename()),
            result.GetLine(),
            result.GetFunction(),
            address);
    }

    return description;
}

X_NAMESPACE_END