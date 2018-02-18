#pragma once

#ifndef _X_HASH_FNV1A_
#define _X_HASH_FNV1A_

X_NAMESPACE_BEGIN(core)

namespace Hash
{
	/// \brief Creates a FNV-1a hash of the input. See -> http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
	/// The algorithm is well suited for hashing nearly identical strings such as URLs, hostnames, filenames, text, IP addresses, etc.
	typedef uint32_t Fnv1Val;
	typedef uint32_t Fnv1aVal;


	Fnv1Val Fnv1Hash(const void* key, size_t length);
	Fnv1aVal Fnv1aHash(const void* key, size_t length);

	namespace Int64
	{
		typedef uint64_t Fnv1Val;
		typedef uint64_t Fnv1aVal;

		Fnv1Val Fnv1Hash(const void* key, size_t length);
		Fnv1aVal Fnv1aHash(const void* key, size_t length);
	}

	// need to organise this a bit better.
	// ideally I don't want these to be used at runtime.
	namespace Fnv1aConst
	{
		constexpr static uint32_t default_offset_basis = 2166136261u;
		constexpr static uint32_t prime = 16777619u;

		namespace Internal
		{
			constexpr static inline Fnv1aVal Hash(char const*const pStr, const uint32_t val)
			{
				return !*pStr ? val : Hash(pStr + 1, static_cast<uint32_t>(((val ^ *pStr) * static_cast<uint64_t>(prime)) & 0xFFFFFFFF));
			}

			constexpr static inline Fnv1aVal Hash(char const*const pStr, const size_t strLen, const uint32_t val)
			{
				return (strLen == 0) ? val : Hash(pStr + 1, strLen - 1, static_cast<uint32_t>(((val ^ *pStr) * static_cast<uint64_t>(prime)) & 0xFFFFFFFF));
			}
		} // namespace Internal

		constexpr static inline Fnv1aVal Hash(char const*const pStr)
		{
			return !*pStr ? default_offset_basis : Internal::Hash(pStr, default_offset_basis);
		}

		constexpr static inline Fnv1aVal Hash(char const*const pStr, const size_t strLen)
		{
			return (strLen == 0) ? default_offset_basis : Internal::Hash(pStr, strLen, default_offset_basis);
		}


	} // namespace Fnv1aConst

	namespace Fnv1Const
	{
		constexpr static uint32_t default_offset_basis = 2166136261u;
		constexpr static uint32_t prime = 16777619u;

		namespace Internal
		{
			constexpr inline Fnv1aVal Hash(char const*const pStr, const uint32_t val)
			{
				return !*pStr ? val : Hash(pStr + 1, static_cast<uint32_t>((*pStr ^ (val * static_cast<uint64_t>(prime))) & 0xFFFFFFFF));
			}

			constexpr inline Fnv1aVal Hash(char const*const pStr, const size_t strLen, const uint32_t val)
			{
				return (strLen == 0) ? val : Hash(pStr + 1, strLen - 1, static_cast<uint32_t>((*pStr ^ (val * static_cast<uint64_t>(prime))) & 0xFFFFFFFF));
			}
		} // namespace Internal

		constexpr inline Fnv1aVal Hash(char const*const pStr)
		{
			return !*pStr ? default_offset_basis : Internal::Hash(pStr, default_offset_basis);
		}

		constexpr inline Fnv1aVal Hash(char const*const pStr, const size_t strLen)
		{
			return (strLen == 0) ? default_offset_basis : Internal::Hash(pStr, strLen, default_offset_basis);
		}

	} // namespace Fnv1aConst

	namespace Fnv1Literals
	{
		inline constexpr uint32_t operator"" _fnv1a(const char* const pStr, const size_t strLen)
		{
			return Fnv1aConst::Internal::Hash(pStr, strLen, Fnv1aConst::default_offset_basis);
		}

		inline constexpr uint32_t operator"" _fnv1(const char* const pStr, const size_t strLen)
		{
			return Fnv1Const::Internal::Hash(pStr, strLen, Fnv1Const::default_offset_basis);
		}
	}
}

X_NAMESPACE_END

#endif // _X_HASH_FNV1A_