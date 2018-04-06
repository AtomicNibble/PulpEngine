#pragma once

#ifndef X_PLATFORM_CLIPBOARD_H_
#define X_PLATFORM_CLIPBOARD_H_

X_NAMESPACE_BEGIN(core)

namespace clipboard
{
    typedef char ClipBoardBuffer[4096];

    bool setText(const char* pBegin, const char* pEnd);

    const char* getText(ClipBoardBuffer& bufOut);

} // namespace clipboard

X_NAMESPACE_END

#endif // !X_PLATFORM_CLIPBOARD_H_