#include "EngineCommon.h"
#include "Random\CryptRand.h"

#include <Wincrypt.h>

X_NAMESPACE_BEGIN(core)

namespace random
{

	CryptRand::CryptRand() :
		hProvider_(0)
	{

	}

	CryptRand::~CryptRand()
	{
		if (hProvider_ != 0) {
			if (!::CryptReleaseContext(hProvider_, 0)) {
				X_ERROR("Rand", "Failed to release contex");
			}
		}
	}

	bool CryptRand::init(void)
	{
		static_assert(sizeof(hProvider_) == sizeof(HCRYPTPROV), "invalid size");

		X_ASSERT(hProvider_ == 0, "Provider already created")(hProvider_);

		if (!CryptAcquireContextW(
			(HCRYPTPROV*)&hProvider_,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT | CRYPT_SILENT
		)) {
			lastError::Description Dsc;
			X_FATAL("Rand", "Failed to acquire contex. Err: %s", lastError::ToString(Dsc));
			return false;
		}

		return true;
	}
	
	void CryptRand::genBytes(uint8_t* pBuf, size_t numBytes)
	{
		X_ASSERT(hProvider_ != 0, "Invalid provider")(hProvider_);

		if (!::CryptGenRandom(hProvider_, safe_static_cast<DWORD>(numBytes), pBuf))
		{
			lastError::Description Dsc;
			X_FATAL("Rand", "Failed to generate random bytes. Err: %s", lastError::ToString(Dsc));
		}
	}

} // namespace random

X_NAMESPACE_END
