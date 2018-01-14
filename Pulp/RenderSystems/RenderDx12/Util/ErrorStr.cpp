#include "stdafx.h"
#include "ErrorStr.h"


X_NAMESPACE_BEGIN(render)


namespace Error
{
	
	const char* ToString(HRESULT error, Description& desc)
	{
		// i want to prefix str with hex value.
		const size_t prefixLen = sizeof("(0x00000000) ") - 1;

		const auto len = sprintf_s(desc, prefixLen + 1, "(0x%8" PRIx32 ") ", error);
		X_ASSERT(len == prefixLen, "Prefix size mismtach")(len, prefixLen);

		DWORD size = ::FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			static_cast<DWORD>(error),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			desc + prefixLen,
			sizeof(Description) - prefixLen,
			NULL
		);

		if (size > 1) {
			auto sizeOffset = size + prefixLen;
			desc[sizeOffset - 2] = '\0';
		}

		return desc;
	}

}


X_NAMESPACE_END;