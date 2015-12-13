#include "stdafx.h"
#include "XNullfont.h"

X_NAMESPACE_BEGIN(font)

#if defined(X_USE_NULLFONT)

XFontNull XFontSysNull::nullFont_;

#else

X_DISABLE_EMPTY_FILE_WARNING

#endif // X_USE_NULLFONT


X_NAMESPACE_END