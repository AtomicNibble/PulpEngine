#pragma once


#ifndef X_ENCRYPTION_SALSA20_H_
#define X_ENCRYPTION_SALSA20_H_

X_NAMESPACE_BEGIN(core)

namespace Encryption
{
	class Salsa20
	{
	public:
		enum : size_t
		{
			VECTOR_SIZE = 16,
			BLOCK_SIZE = 64,
			KEY_SIZE = 32,
			IV_SIZE = 8
		};

		Salsa20(const uint8_t* key);
		Salsa20(const Salsa20&);
		~Salsa20();
		Salsa20& operator=(const Salsa20&);

		void setKey(const uint8_t* key);
		void setIv(const uint8_t* iv);
		void generateKeyStream(uint8_t output[BLOCK_SIZE]);
		void processBlocks(const uint8_t* input, uint8_t* output, size_t numBlocks);
		void processBytes(const uint8_t* input, uint8_t* output, size_t numBytes);

	private:
		inline uint32_t rotate(uint32_t value, uint32_t numBits);
		inline void convert(uint32_t value, uint8_t* array);
		inline uint32_t convert(const uint8_t* array);

		uint32_t vector_[VECTOR_SIZE];
	};

} // namespace Encryption

X_NAMESPACE_END

#endif // X_ENCRYPTION_SALSA20_H_