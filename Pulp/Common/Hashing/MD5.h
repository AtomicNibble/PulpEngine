#pragma once

#ifndef X_HASH_MD5_H_
#define X_HASH_MD5_H_

#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

	typedef Digest<16> MD5Digest;

	class MD5
	{
	public:
		MD5();
		~MD5();

		void reset(void);
		void update(const uint8_t* buf, size_t length);
		void update(const char* buf, size_t length);
		void update(const char* str);

		MD5Digest& finalize(void);

	private:
		static const int blocksize = 64;

		void transform(const uint8_t block[blocksize]);
		static void decode(uint32_t* output, const uint8_t* input, size_t len);
		static void encode(uint8_t* output, const uint32_t* input, size_t len);

	private:
		uint32_t state_[4];
		uint32_t count_[2]; // bits
		uint8_t buffer_[blocksize];

		bool finalized_;

		MD5Digest digest_;
	};


} // namespace Hash

X_NAMESPACE_END

#endif // !X_HASH_MD5_H_