#include "stdafx.h"




#include <Hashing\crc32.h>
#include <Hashing\Fnva1Hash.h>
#include <Hashing\MurmurHash.h>
#include <Hashing\sha1.h>
#include <Hashing\sha512.h>
#include <Hashing\MD5.h>
#include "Hashing\Adler32.h"
#include <Hashing\xxHash.h>

#include <Util\EndianUtil.h>

X_USING_NAMESPACE;

using namespace core;
using namespace core::Hash;


TEST(Hash, crc32) {

	Crc32 crc;

	uint32 val = crc.GetCRC32("my name is wincat");
	uint32 val2 = crc.GetCRC32("my name is Wincat");

	EXPECT_EQ(0x4b2da119, val);
	EXPECT_EQ(0x4c81a42f, val2);

	EXPECT_FALSE(crc.GetCRC32("hello") == crc.GetCRC32("HELLO"));
	EXPECT_TRUE(crc.GetCRC32("hello") == crc.GetCRC32Lowercase("HELLO"));

}

TEST(Hash, Fnva1) {

	uint32 hash = Fnv1aHash("readme.txt", 10);
	uint64 hash64 = Int64::Fnv1aHash("readme.txt", 10);

	EXPECT_TRUE(hash == 0x966dcb41);
	EXPECT_TRUE(hash64 == 0xdaca8649b3b4bd97);

	EXPECT_FALSE(hash == Fnv1aHash("readme.txt.", 11));
}

TEST(Hash, Fnva1Const) {

	const uint32 hash = Fnv1aConst::Hash("readme.txt", 10);
	const uint32 hash1 = Fnv1aConst::Hash("fat goat", 8);

	EXPECT_TRUE(hash == 0x966dcb41);
	EXPECT_TRUE(hash == Fnv1aHash("readme.txt", 10)); // check againstt runtime version.
	EXPECT_FALSE(hash == Fnv1aConst::Hash("readme.txt.", 11));

	EXPECT_TRUE(hash1 == 0xCC731FDF);
	EXPECT_TRUE(hash1 == Fnv1aHash("fat goat", 8)); // check againstt runtime version.

	// compile time.
	static_assert(Fnv1aConst::Hash("readme.txt") == 0x966dcb41, "FnvaConst hash failure");
	static_assert(Fnv1aConst::Hash("fat goat") == 0xCC731FDF, "FnvaConst hash failure");

	using namespace core::Hash::Literals;

	static_assert(Fnv1aConst::Hash("readme.txt") == "readme.txt"_fnv1a, "FnvaConst hash failure");
	static_assert(Fnv1aConst::Hash("fat goat") == "fat goat"_fnv1a, "FnvaConst hash failure");

	EXPECT_TRUE(Fnv1aHash("readme.txt", 10) == "readme.txt"_fnv1a);
	EXPECT_TRUE(Fnv1aHash("fat goat", 8) == "fat goat"_fnv1a);
}

TEST(Hash, Fnva) {

	uint32 hash = Fnv1Hash("readme.txt", 10);
	uint64 hash64 = Int64::Fnv1Hash("readme.txt", 10);

	EXPECT_TRUE(hash == 0x7b0fccd9);
	EXPECT_TRUE(hash64 == 0xb0713f900802e0af);

	EXPECT_FALSE(hash == Fnv1aHash("readme.txt.", 10));
}

TEST(Hash, FnvaConst) {

	const uint32 hash = Fnv1Const::Hash("readme.txt", 10);
	const uint32 hash1 = Fnv1Const::Hash("fat goat", 8);

	EXPECT_TRUE(hash == 0x7b0fccd9);
	EXPECT_TRUE(hash == Fnv1Hash("readme.txt", 10)); // check againstt runtime version.
	EXPECT_FALSE(hash == Fnv1Const::Hash("readme.txt.", 11));

	EXPECT_TRUE(hash1 == 0x9776cf9f);
	EXPECT_TRUE(hash1 == Fnv1Hash("fat goat", 8)); // check againstt runtime version.

	// compile time.
	static_assert(Fnv1Const::Hash("readme.txt") == 0x7b0fccd9, "FnvaConst hash failure");
	static_assert(Fnv1Const::Hash("fat goat") == 0x9776cf9f, "FnvaConst hash failure");

	using namespace core::Hash::Literals;

	static_assert(Fnv1Const::Hash("readme.txt") == "readme.txt"_fnv1, "FnvaConst hash failure");
	static_assert(Fnv1Const::Hash("fat goat") == "fat goat"_fnv1, "FnvaConst hash failure");

	EXPECT_TRUE(Fnv1Hash("readme.txt", 10) == "readme.txt"_fnv1);
	EXPECT_TRUE(Fnv1Hash("fat goat", 8) == "fat goat"_fnv1);
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


TEST(Hash, MD5)
{
	Hash::MD5Digest val, expected;
	Hash::MD5Digest::String str;

	expected.data[0] = 0x0307ed05;
	expected.data[1] = 0xbb095b54;
	expected.data[2] = 0xc60e9c12;
	expected.data[3] = 0xcf921e78;

	Hash::MD5 md5;
	md5.update("tickle my pickle");
	val = md5.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("05ed0703545b09bb129c0ec6781e92cf", str);
}

TEST(Hash, SHA1)
{
	Hash::SHA1Digest val, expected;
	Hash::SHA1Digest::String str;

	expected.data[0] = 0x2ed5f43d;
	expected.data[1] = 0x5d38c6d7;
	expected.data[2] = 0xe329f2aa;
	expected.data[3] = 0x3f04d422;
	expected.data[4] = 0xe25e92fe;

	Hash::SHA1 sha1;
	sha1.update("hash me baby!");
	val = sha1.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("3df4d52ed7c6385daaf229e322d4043ffe925ee2", str);
}

TEST(Hash, SHA1_blank)
{
	Hash::SHA1Digest val, expected;
	Hash::SHA1Digest::String str;

	expected.data[0] = Endian::swap(0xda39a3ee);
	expected.data[1] = Endian::swap(0x5e6b4b0d);
	expected.data[2] = Endian::swap(0x3255bfef);
	expected.data[3] = Endian::swap(0x95601890);
	expected.data[4] = Endian::swap(0xafd80709);

	Hash::SHA1 sha1;
	sha1.update("");
	val = sha1.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("da39a3ee5e6b4b0d3255bfef95601890afd80709", str);
}

TEST(Hash, SHA1_896)
{
	Hash::SHA1Digest val, expected;
	Hash::SHA1Digest::String str;

	expected.data[0] = Endian::swap(0xa49b2446);
	expected.data[1] = Endian::swap(0xa02c645b);
	expected.data[2] = Endian::swap(0xf419f995);
	expected.data[3] = Endian::swap(0xb6709125);
	expected.data[4] = Endian::swap(0x3a04a259);

	Hash::SHA1 sha1;
	sha1.update("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghij"
		"klmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
	val = sha1.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("a49b2446a02c645bf419f995b67091253a04a259", str);
}


TEST(Hash, SHA512_blank)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0xcf83e135);
	expected.data[1] = Endian::swap(0x7eefb8bd);
	expected.data[2] = Endian::swap(0xf1542850);
	expected.data[3] = Endian::swap(0xd66d8007);
	expected.data[4] = Endian::swap(0xd620e405);
	expected.data[5] = Endian::swap(0x0b5715dc);
	expected.data[6] = Endian::swap(0x83f4a921);
	expected.data[7] = Endian::swap(0xd36ce9ce);
	expected.data[8] = Endian::swap(0x47d0d13c);
	expected.data[9] = Endian::swap(0x5d85f2b0);
	expected.data[10] = Endian::swap(0xff8318d2);
	expected.data[11] = Endian::swap(0x877eec2f);
	expected.data[12] = Endian::swap(0x63b931bd);
	expected.data[13] = Endian::swap(0x47417a81);
	expected.data[14] = Endian::swap(0xa538327a);
	expected.data[15] = Endian::swap(0xf927da3e);

	Hash::SHA512 sha512;
	sha512.update("");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a9"
		"21d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e", str);
}

TEST(Hash, SHA512_abc)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0xddaf35a1);
	expected.data[1] = Endian::swap(0x93617aba);
	expected.data[2] = Endian::swap(0xcc417349);
	expected.data[3] = Endian::swap(0xae204131);
	expected.data[4] = Endian::swap(0x12e6fa4e); 
	expected.data[5] = Endian::swap(0x89a97ea2);
	expected.data[6] = Endian::swap(0x0a9eeee6); 
	expected.data[7] = Endian::swap(0x4b55d39a);
	expected.data[8] = Endian::swap(0x2192992a); 
	expected.data[9] = Endian::swap(0x274fc1a8);
	expected.data[10] = Endian::swap(0x36ba3c23); 
	expected.data[11] = Endian::swap(0xa3feebbd);
	expected.data[12] = Endian::swap(0x454d4423);
	expected.data[13] = Endian::swap(0x643ce80e);
	expected.data[14] = Endian::swap(0x2a9ac94f); 
	expected.data[15] = Endian::swap(0xa54ca49f);

	Hash::SHA512 sha512;
	sha512.update("abc");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b5"
		"5d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", str);
}

TEST(Hash, SHA512_896)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0x8e959b75);
	expected.data[1] = Endian::swap(0xdae313da);
	expected.data[2] = Endian::swap(0x8cf4f728);
	expected.data[3] = Endian::swap(0x14fc143f);
	expected.data[4] = Endian::swap(0x8f7779c6);
	expected.data[5] = Endian::swap(0xeb9f7fa1);
	expected.data[6] = Endian::swap(0x7299aead);
	expected.data[7] = Endian::swap(0xb6889018);
	expected.data[8] = Endian::swap(0x501d289e);
	expected.data[9] = Endian::swap(0x4900f7e4);
	expected.data[10] = Endian::swap(0x331b99de);
	expected.data[11] = Endian::swap(0xc4b5433a);
	expected.data[12] = Endian::swap(0xc7d329ee);
	expected.data[13] = Endian::swap(0xb6dd2654);
	expected.data[14] = Endian::swap(0x5e96e55b);
	expected.data[15] = Endian::swap(0x874be909);

	Hash::SHA512 sha512;
	sha512.update("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
		"jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb688901"
		"8501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909", str);
}


TEST(Hash, SHA512_1000000)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0xe718483d);
	expected.data[1] = Endian::swap(0x0ce76964);
	expected.data[2] = Endian::swap(0x4e2e42c7);
	expected.data[3] = Endian::swap(0xbc15b463);
	expected.data[4] = Endian::swap(0x8e1f98b1);
	expected.data[5] = Endian::swap(0x3b204428);
	expected.data[6] = Endian::swap(0x5632a803);
	expected.data[7] = Endian::swap(0xafa973eb);
	expected.data[8] = Endian::swap(0xde0ff244);
	expected.data[9] = Endian::swap(0x877ea60a);
	expected.data[10] = Endian::swap(0x4cb0432c);
	expected.data[11] = Endian::swap(0xe577c31b);
	expected.data[12] = Endian::swap(0xeb009c5c);
	expected.data[13] = Endian::swap(0x2c49aa2e);
	expected.data[14] = Endian::swap(0x4eadb217);
	expected.data[15] = Endian::swap(0xad8cc09b);

	Hash::SHA512 sha512;

	for (size_t i = 0; i < 1000000; i++)
	{
		sha512.update("a");
	}

	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973e"
		"bde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b", str);
}




TEST(Hash, Adler32)
{
	Hash::Adler32Val val;

	val = Adler32("hello, can you fart in a cart?");

	EXPECT_EQ(val, 0xa3050a5e);
}


TEST(Hash, xxhash32)
{
	Hash::xxHash32 hasher;

	hasher.reset(12345);
	hasher.update("test", 4);

	EXPECT_EQ(3834992036, hasher.finalize());

	hasher.reset(123);
	hasher.update("test", 4);

	EXPECT_EQ(2758658570, hasher.finalize());
}

TEST(Hash, xxhash64)
{
	Hash::xxHash64 hasher;

	hasher.reset(123);
	hasher.update("test", 4);

	EXPECT_EQ(3134990500624303823, hasher.finalize());

	hasher.reset(0);
	hasher.update("test", 4);

	EXPECT_EQ(5754696928334414137, hasher.finalize());
}



TEST(Hash, xxhash32_simple)
{
	EXPECT_EQ(3834992036, Hash::xxHash32::getHash("test", 4, 12345));
	EXPECT_EQ(2758658570, Hash::xxHash32::getHash("test", 4, 123));
}

TEST(Hash, xxhash64_simple)
{
	EXPECT_EQ(3134990500624303823, Hash::xxHash64::getHash("test", 4, 123));
	EXPECT_EQ(5754696928334414137, Hash::xxHash64::getHash("test", 4, 0));
}