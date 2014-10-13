#include "EngineCommon.h"
#include "Util\LastError.h"

X_NAMESPACE_BEGIN(core)


namespace lastError
{
	unsigned int Get(void) {
		return ::GetLastError();
	}

	const char* ToString( DWORD error, Description& desc )
	{
		DWORD size = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,			// It�s a system error
			NULL,											// No string to be formatted needed
			error,											// Hey Windows: Please explain this error!
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),		// Do it in the standard language
			desc,											// Put the message here
			sizeof( Description ),							// Number of bytes to store the message
			NULL);

		desc[size-2] = '\0';

		return desc;
	}

	const char* ToString(Description& desc)
	{
		return ToString( ::GetLastError(), desc );
	}
}


X_NAMESPACE_END;