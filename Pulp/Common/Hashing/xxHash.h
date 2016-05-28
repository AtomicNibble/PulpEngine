#pragma once


X_NAMESPACE_BEGIN(core)

namespace Hash
{
	// Version	Speed on 64-bits	Speed on 32-bits
	// XXH64	13.8 GB / s			1.9 GB / s
	// XXH32	6.8 GB / s			6.0 GB / s

	class xxHash32
	{
	public:
		xxHash32();
		~xxHash32() = default;

		void reset(uint32_t seed);
		bool update(const void* pBuf, size_t length);
		uint32_t finalize(void);

		static uint32_t getHash(const void* pInput, size_t length, uint32_t seed);
	
	private:
		struct { long long ll[6]; } state_;
	};


	class xxHash64
	{
	public:
		xxHash64();
		~xxHash64() = default;

		void reset(uint64_t seed);
		bool update(const void* pBuf, size_t length);
		uint64_t finalize(void);

		static uint64_t getHash(const void* pInput, size_t length, uint64_t seed);

	private:
		struct { long long ll[11]; } state_;
	};



} // namespace Hash

X_NAMESPACE_END