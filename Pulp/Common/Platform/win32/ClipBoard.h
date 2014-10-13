#pragma once


#ifndef X_PLATFORM_CLIPBOARD_H_
#define X_PLATFORM_CLIPBOARD_H_

X_NAMESPACE_BEGIN(core)


namespace clipboard
{
	// not thread safe
	bool setText(const char* pStr);

	// not thread safe
	const char* getText(void);

}


X_NAMESPACE_END

#endif // !X_PLATFORM_CLIPBOARD_H_