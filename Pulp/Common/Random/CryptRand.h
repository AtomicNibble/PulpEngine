#pragma once


X_NAMESPACE_BEGIN(core)

namespace random
{

	class CryptRand
	{
	public:
		CryptRand();
		~CryptRand();
		
		bool init(void);
		
		void genBytes(uint8_t* pBuf, size_t numBytes);

	private:
		uintptr_t hProvider_;
	};

} // namespace random

X_NAMESPACE_END
