#pragma once


#ifndef X_HASH_SHA1_H_
#define X_HASH_SHA1_H_

X_NAMESPACE_BEGIN(core)

namespace Hash
{
	struct Sha1Hash
	{
		typedef char TextValue[41];

		UINT64   nTotal;
		UINT32   H[5];
		UINT8    block[64];
		size_t   nblock;

		bool operator==(const Sha1Hash& oth) const {
			return memcmp(H, &oth.H, 20) == 0;
		}

		bool operator!=(const Sha1Hash& oth) const {
			return !(*this == oth);
		}
	};


	void Sha1Init(Sha1Hash& hash);
	void Sha1Update(Sha1Hash& hash, const void *data, size_t n);
	void Sha1Final(Sha1Hash& hash);
	void Sha1ToString(const Sha1Hash& hash, Sha1Hash::TextValue& hexstring);
}


X_NAMESPACE_END

#endif // X_HASH_SHA1_H_