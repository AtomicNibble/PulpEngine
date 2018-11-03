#include "EngineCommon.h"
#include "ClipBoard.h"

X_NAMESPACE_BEGIN(core)

namespace clipboard
{
    bool setText(const char* pBegin, const char* pEnd)
    {
        core::lastError::Description Dsc;

        X_ASSERT(pBegin <= pEnd, "Invalid range")(pBegin, pEnd);

        const size_t strLen = union_cast<size_t>(pEnd - pBegin);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strLen + 1);
        LPSTR pDst = reinterpret_cast<LPSTR>(GlobalLock(hMem));

        std::memcpy(pDst, pBegin, strLen);
        pDst[strLen + 1] = '\0';

        if (GlobalUnlock(hMem) != 0 && core::lastError::Get() != ERROR_NOT_LOCKED) {
            X_WARNING("Clipboard", "failed to unlock clipboard memory. Error: %s", core::lastError::ToString(Dsc));
            return false;
        }

        if (!OpenClipboard(NULL)) {
            X_WARNING("Clipboard", "failed to open clipboard. Error: %s", core::lastError::ToString(Dsc));
            return false;
        }

        // empty it.
        if (!EmptyClipboard()) {
            X_WARNING("Clipboard", "failed to EmptyClipboard. Error: %s", core::lastError::ToString(Dsc));
            CloseClipboard();
            return false;
        }

        if (!SetClipboardData(CF_TEXT, hMem)) {
            X_WARNING("Clipboard", "failed to SetClipboardData. Error: %s", core::lastError::ToString(Dsc));
            CloseClipboard();
            return false;
        }

        if (!CloseClipboard()) {
            X_WARNING("Clipboard", "failed to CloseClipboard. Error: %s", core::lastError::ToString(Dsc));
        }

        if (strLen > 512) {
            X_LOG1("Clipboard", "set cliboard: %.*s...", 512, pBegin);
        }
        else {
            X_LOG1("Clipboard", "set cliboard: %.*s", strLen, pBegin);
        }
        return true;
    }

    const char* getText(ClipBoardBuffer& bufOut)
    {
        core::lastError::Description Dsc;

        if (!IsClipboardFormatAvailable(CF_TEXT)) {
            X_WARNING("Clipboard", "unable to get cliboard text data. Error: %s", core::lastError::ToString(Dsc));
            return nullptr;
        }

        if (!OpenClipboard(NULL)) {
            X_WARNING("Clipboard", "failed to open clipboard. Error: %s", core::lastError::ToString(Dsc));
            return nullptr;
        }

        HGLOBAL hGlobal = GetClipboardData(CF_TEXT);
        if (!hGlobal) {
            X_WARNING("Clipboard", "failed to get clipboard data. Error: %s", core::lastError::ToString(Dsc));
            return nullptr;
        }

        void* pGlobal = GlobalLock(hGlobal);

        size_t strLength = strlen(reinterpret_cast<char*>(pGlobal));
        if (strLength > sizeof(bufOut) - 1) {
            X_WARNING("Clipboard", "Clipboard text data is over %i bytes. truncating string", sizeof(bufOut));
        }

        strLength = core::Min(sizeof(bufOut) - 1, strLength);

        // make sure it's null termed.
        bufOut[strLength] = '\0';

        // copy it shit face.
        std::memcpy(bufOut, pGlobal, strLength);

        if (!GlobalUnlock(hGlobal)) {
            X_WARNING("Clipboard", "failed to unlock global data. Error: %s", core::lastError::ToString(Dsc));
        }

        if (!CloseClipboard()) {
            X_WARNING("Clipboard", "failed to CloseClipboard. Error: %s", core::lastError::ToString(Dsc));
        }

        return bufOut;
    }

} // namespace clipboard

X_NAMESPACE_END