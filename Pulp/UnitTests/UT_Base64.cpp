#include "stdafx.h"
#include "gtest\gtest.h"
#include "String\Base64.h"

X_USING_NAMESPACE;

using namespace core;


TEST(Base64, Encode)
{
	core::string encoded = Base64::Encode("my booty is firm and plump");

	EXPECT_STREQ("bXkgYm9vdHkgaXMgZmlybSBhbmQgcGx1bXA=", encoded.c_str());
}


TEST(Base64, Decode)
{
	core::string decoded = Base64::Decode("bXkgYm9vdHkgaXMgZmlybSBhbmQgcGx1bXA=");

	EXPECT_STREQ("my booty is firm and plump", decoded.c_str());
}