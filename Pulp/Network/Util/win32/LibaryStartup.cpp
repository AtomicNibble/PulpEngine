#include "stdafx.h"
#include "LibaryStartup.h"

X_NAMESPACE_BEGIN(net)

namespace PlatLib
{
	namespace
	{

		core::AtomicInt refCount(0);

	} // namespace


	bool isStarted(void)
	{
		return refCount > 0;
	}

	bool addRef(void)
	{
		if (++refCount == 1)
		{
			platform::WSADATA winsockInfo;
			if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0)
			{
				--refCount; // we don't need a matching cleanup call.

				lastError::Description Dsc;
				X_ERROR("Net", "Failed to init winsock. Error: \"%s\"", lastError::ToString(Dsc));
				return false;
			}
		}

		return true;
	}

	void deRef(void)
	{
		// check ref is positive.
		if (refCount > 0 && --refCount == 0)
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
