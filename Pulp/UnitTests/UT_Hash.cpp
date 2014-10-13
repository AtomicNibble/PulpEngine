#include "stdafx.h"

#include "gtest/gtest.h"


#include <Hashing\crc32.h>
#include <Hashing\Fnva1Hash.h>
#include <Hashing\MurmurHash.h>
#include <Hashing\sha1.h>


X_USING_NAMESPACE;

using namespace core;
using namespace core::Hash;


TEST(Hash, crc32) {

	Crc32 crc;

	uint32 val = crc.GetCRC32("my name is wincat");
	uint32 val2 = crc.GetCRC32("my name is Wincat");

	EXPECT_TRUE(val == 0x4b2da119);
	EXPECT_TRUE(val2 == 0x4c81a42f);

	EXPECT_FALSE(crc.GetCRC32("hello") == crc.GetCRC32("HELLO"));
	EXPECT_TRUE(crc.GetCRC32("hello") == crc.GetCRC32Lowercase("HELLO"));

}

TEST(Hash, Fnva) {

	uint32 hash = Fnv1aHash("readme.txt", 10);
	uint64 hash64 = Int64::Fnv1aHash("readme.txt", 10);

	EXPECT_TRUE(hash == 0x966dcb41);
	EXPECT_TRUE(hash64 == 0xdaca8649b3b4bd97);

	EXPECT_FALSE(hash == Fnv1aHash("readme.txt.", 11));
}

TEST(Hash, Murmur) {


	uint32 val = 0x131;
	uint32 val2 = 0x132;

	uint32 hash = Hash::MurmurHash2(&val, sizeof(val), 0);
	uint32 hash2 = Hash::MurmurHash2(&val2, sizeof(val2), 0);


	EXPECT_EQ(hash, 0xe6e00531);
	EXPECT_EQ(hash2, 0x87c86240);
	EXPECT_NE(hash,hash2);
}


TEST(Hash, Sha1)
{
	Hash::Sha1Hash val, expected;
	Hash::Sha1Hash::TextValue str;

	expected.H[0] = 0x3df4d52e;
	expected.H[1] = 0xd7c6385d;
	expected.H[2] = 0xaaf229e3;
	expected.H[3] = 0x22d4043f;
	expected.H[4] = 0xfe925ee2;

	Hash::Sha1Init(val);
	Hash::Sha1Update(val, "hash me baby!", 13);
	Hash::Sha1Final(val);

	Hash::Sha1ToString(val, str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("3df4d52ed7c6385daaf229e322d4043ffe925ee2", str);
}