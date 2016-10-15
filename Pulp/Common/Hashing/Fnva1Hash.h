#pragma once

#ifndef _X_HASH_FNV1A_
#define _X_HASH_FNV1A_

X_NAMESPACE_BEGIN(core)

namespace Hash
{
	/// \brief Creates a FNV-1a hash of the input. See -> http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
	/// \Details We use -1a version as it provides better dispersion 
	/// The algorithm is well suited for hashing nearly identical strings such as URLs, hostnames, filenames, text, IP addresses, etc.
	typedef uint32_t Fnv1aVal;


	Fnv1aVal Fnv1aHash(const void* key, size_t length);

	namespace Int64
	{
		typedef uint64_t Fnv1aVal;

		Fnv1aVal Fnv1aHash(const void* key, size_t length);
	}

	// need to organise this a bit better.
	// ideally I don't want these to be used at runtime.
	namespace Fnv1aConst
	{
		constexpr static uint32_t default_offset_basis = 0x811C9DC5;
		constexpr static uint32_t prime = 0x01000193;

		namespace Internal
		{
			constexpr static inline Fnv1aVal Hash(char const*const pStr, const uint32_t val)
			{
				return !*pStr ? val : Hash(pStr + 1, static_cast<uint32_t>((val ^ *pStr) * static_cast<uint64_t>(prime)));
			}

			constexpr static inline Fnv1aVal Hash(char const*const pStr, const size_t strLen, const uint32_t val)
			{
				return (strLen == 0) ? val : Hash(pStr + 1, strLen - 1, static_cast<uint32_t>((val ^ *pStr) * static_cast<uint64_t>(prime)));
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
}

X_NAMESPACE_END

#endif // _X_HASH_FNV1A_