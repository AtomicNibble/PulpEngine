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
		typedef uint32_t HashVal;

	public:
		xxHash32();
		~xxHash32() = default;

		void reset(uint32_t seed = 0);
		bool update(const void* pBuf, size_t length);
		template<typename T>
		X_INLINE bool update(const T* pType, size_t numT);
		template<typename T>
		X_INLINE bool update(const T& type);

		HashVal finalize(void);

		static HashVal getHash(const void* pInput, size_t length, uint32_t seed = 0);
	
	private:
		struct { long long ll[6]; } state_;
	};


	class xxHash64
	{
	public:
		typedef uint64_t HashVal;

	public:
		xxHash64();
		~xxHash64() = default;

		void reset(uint64_t seed = 0);

		bool update(const void* pBuf, size_t length);
		template<typename T>
		X_INLINE bool update(const T* pType, size_t numT);
		template<typename T>
		X_INLINE bool update(const T& type);

		HashVal finalize(void);

		static HashVal getHash(const void* pInput, size_t length, uint64_t seed = 0);

	private:
		struct { long long ll[11]; } state_;
	};


	template<typename T>
	X_INLINE bool xxHash32::update(const T* pType, size_t numT)
	{
		static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
		return update(static_cast<const void*>(pType), numT * sizeof(T));
	}

	template<typename T>
	X_INLINE bool xxHash32::update(const T& type)
	{
		static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
		return update(static_cast<const void*>(&type), sizeof(T));
	}

	template<typename T>
	X_INLINE bool xxHash64::update(const T* pType, size_t numT)
	{
		static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
		return update(static_cast<const void*>(pType), numT * sizeof(T));
	}

	template<typename T>
	X_INLINE bool xxHash64::update(const T& type)
	{
		static_assert(core::compileTime::IsPOD<T>::Value, "hashing of none POD type");
		return update(static_cast<const void*>(&type), sizeof(T));
	}

} // namespace Hash

X_NAMESPACE_END