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
		uint64_t Fnv1aHash(const void* key, size_t length);
	}
}

X_NAMESPACE_END

#endif // _X_HASH_FNV1A_