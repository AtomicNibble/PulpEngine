#include "EngineCommon.h"
#include "ClipBoard.h"

X_NAMESPACE_BEGIN(core)


namespace clipboard
{
	namespace{

		char g_buf[4096];
	}

	bool setText(const char* pStr)
	{
		core::lastError::Description Dsc;
		size_t strLen;
		HGLOBAL hMem;
		LPSTR pDst;

		strLen = strlen(pStr) + 1;
		hMem = GlobalAlloc(GMEM_MOVEABLE, strLen);
		pDst = (LPSTR)GlobalLock(hMem);

		memcpy(pDst, pStr, strLen);

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
			return false;
		}

		if (!CloseClipboard())
			X_WARNING("Clipboard", "failed to CloseClipboard. Error: %s", core::lastError::ToString(Dsc));

		X_LOG1("Clipboard", "set cliboard: %s", pStr);
		return true;
	}

	const char* getText(void)
	{
		HGLOBAL hGlobal;
		void*  pGlobal;
		size_t strLength;
		core::lastError::Description Dsc;

		if (!IsClipboardFormatAvailable(CF_TEXT)) {
			X_WARNING("Clipboard", "unable to get cliboard text data. Error: %s", core::lastError::ToString(Dsc));
			return nullptr;
		}

		if (!OpenClipboard(NULL)) {
			X_WARNING("Clipboard", "failed to open clipboard. Error: %s", core::lastError::ToString(Dsc));
			return nullptr;
		}

		hGlobal = GetClipboardData(CF_TEXT);

		if (!hGlobal)
			return nullptr;

		pGlobal = GlobalLock(hGlobal);

		strLength = strlen((char*)pGlobal);

		if (strLength > sizeof(g_buf)-1){
			X_WARNING("Clipboard", "Clipboard text data is over %i bytes. truncating string", sizeof(g_buf));
		}

		strLength = core::Min(sizeof(g_buf)-1, strLength);

		// make sure it's null termed.
		g_buf[strLength] = '\0';

		// copy it shit face.
		strncpy(g_buf, (char*)pGlobal, strLength);


		if(!GlobalUnlock(hGlobal))
			X_WARNING("Clipboard", "failed to unlock global data. Error: %s", core::lastError::ToString(Dsc));
		
		if(!CloseClipboard())
			X_WARNING("Clipboard", "failed to CloseClipboard. Error: %s", core::lastError::ToString(Dsc));

		return g_buf;
	}


}


X_NAMESPACE_END