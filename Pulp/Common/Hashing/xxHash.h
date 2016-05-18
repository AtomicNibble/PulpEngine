#pragma once


X_NAMESPACE_BEGIN(core)

namespace Hash
{

//	uint32_t xxHash32(const void* pInput, size_t length, uint32_t seed);
//	uint64_t xxHash64(const void* pInput, size_t length, uint64_t seed);


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

		void reset(uint32_t seed);
		bool update(const void* pBuf, size_t length);
		uint64_t finalize(void);

		static uint64_t getHash(const void* pInput, size_t length, uint64_t seed);

	private:
		struct { long long ll[11]; } state_;
	};



} // namespace Hash

X_NAMESPACE_END