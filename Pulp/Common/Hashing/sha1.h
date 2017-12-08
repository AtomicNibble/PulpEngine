#pragma once


#ifndef X_HASH_SHA1_H_
#define X_HASH_SHA1_H_

#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

	typedef Digest<20> SHA1Digest;

	class SHA1
	{
	public:
		SHA1();
		~SHA1();

		void reset(void);
		void update(const void* pBuf, size_t length);
		void update(const char* pStr);

		template<typename T>
		void update(const T& obj) {
			update(reinterpret_cast<const void*>(&obj), sizeof(T));
		}

		SHA1Digest finalize(void);

	private:
		static const uint32_t DIGEST_INTS = 5;
		static const uint32_t BLOCK_INTS = 16;
		static const uint32_t BLOCK_BYTES = BLOCK_INTS * 4;

	private:
		void transform(const uint8_t* pBuffer);

		uint8_t buffer_[BLOCK_BYTES];

		union
		{
			uint8_t c[BLOCK_BYTES];
			uint32_t l[BLOCK_INTS];
		} block_;

		uint32_t digest_[DIGEST_INTS];
		size_t numBytes_;
	};

} // namespace Hash

X_NAMESPACE_END

#endif // X_HASH_SHA1_H_