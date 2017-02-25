#include "stdafx.h"
#include "LibaryStartup.h"

X_NAMESPACE_BEGIN(net)

namespace PlatLib
{
	namespace
	{

		static int32_t refCount = 0;

	} // namespace


	bool addRef(void)
	{
		++refCount;

		if (refCount == 1)
		{
			platform::WSADATA winsockInfo;
			if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0)
			{
				lastError::Description Dsc;
				X_ERROR("Net", "Failed to init winsock. Error: \"%s\"", lastError::ToString(Dsc));
				return false;
			}
		}

		return true;
	}

	void deRef(void)
	{
		++refCount;

		if (refCount == 0)
		{
			if (platform::WSACleanup() != 0)
			{
				lastError::Description Dsc;
				X_ERROR("Net", "Failed to cleanup winsock. Error: \"%s\"", lastError::ToString(Dsc));
			}
		}
	}

	// -----------------

	ScopedRef::ScopedRef()
	{
		addRef();
	}

	ScopedRef::~ScopedRef()
	{
		deRef();
	}

} // namespace PlatLib

X_NAMESPACE_END
