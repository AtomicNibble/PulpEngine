#include "stdafx.h"
#include "LastErrorWSA.h"

X_NAMESPACE_BEGIN(net)


namespace lastErrorWSA
{
	int32_t Get(void) 
	{
		return platform::WSAGetLastError();
	}

	const char* ToString(int32_t error, Description& desc)
	{
		DWORD size = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,			// It´s a system error
			NULL,											// No string to be formatted needed
			error,											// Hey Windows: Please explain this error!
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),		// Do it in the standard language
			desc,											// Put the message here
			sizeof(Description),							// Number of bytes to store the message
			NULL);

		if (size > 1) {
			desc[size - 2] = '\0';
		}
		else {
			desc[0] = '\0';
		}

		return desc;
	}

	const char* ToString(Description& desc)
	{
		return ToString(Get(), desc);
	}
}


X_NAMESPACE_END;