#pragma once


#ifndef X_ENCRYPTION_SALSA20_H_
#define X_ENCRYPTION_SALSA20_H_

X_NAMESPACE_BEGIN(core)

namespace Encryption
{
#define SALSA20_SSE 1

	class Salsa20
	{
		static const int32_t NUM_ROUNDS = 12;
	public:
		enum : size_t
		{
			VECTOR_SIZE = 16,
			BLOCK_SIZE = 64,
			KEY_SIZE = 32,
			IV_SIZE = 8
		};

		typedef uint8_t Key[KEY_SIZE];
		typedef uint8_t Iv[IV_SIZE];

		Salsa20();
		explicit Salsa20(const uint8_t* key);
		Salsa20(const Salsa20& oth);
		~Salsa20();
		Salsa20& operator=(const Salsa20&);

		void setKey(const uint8_t* key);
		void setIv(const uint8_t* iv);
		void generateKeyStream(uint8_t output[BLOCK_SIZE]);
#if SALSA20_SSE
		void generateKeyStreamSSEOrdering(uint8_t output[BLOCK_SIZE]);
#endif // SALSA20_SSE

		void processBlocks(const uint8_t* input, uint8_t* output, size_t numBlocks);
		void processBytes(const uint8_t* input, uint8_t* output, size_t numBytes);
		void processBytes(const uint8_t* input, uint8_t* output, size_t numBytes, int64_t byteOffset);

	private:
		static bool SSEnabled(void);
#if SALSA20_SSE
		static bool SSESupported(void);

		void processBytesSSE(const uint8_t* input, uint8_t* output,
			size_t numBytes, int64_t byteOffset);
#endif // SALSA20_SSE

	private:
		inline uint32_t rotate(uint32_t value, uint32_t numBits);
		inline void convert(uint32_t value, uint8_t* array);
		inline uint32_t convert(const uint8_t* array);

		union
		{
#if SALSA20_SSE
			__m128i vectorSSE_[4];
#endif // SALSA20_SSE
			uint32_t vector_[VECTOR_SIZE];
		};

#if SALSA20_SSE
		static __m128i s_maskLo32;
		static __m128i s_maskHi32;

		struct SSECheckState
		{
			enum Enum
			{
				NOT_CHECKED,
				NOT_SUPPORTED,
				SUPPORTED
			};
		};

		static SSECheckState::Enum s_SSEState_;
		static Spinlock s_checkLock;
#endif // SALSA20_SSE

	};

} // namespace Encryption

X_NAMESPACE_END

#endif // X_ENCRYPTION_SALSA20_H_