#include "stdafx.h"
#include "gtest/gtest.h"

#include <String\Path.h>

X_USING_NAMESPACE;


TEST(Path, Construct)
{
	core::Path<char> path;

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	EXPECT_EQ(260, path.capacity());
	EXPECT_EQ(0, path.length());

	path.ensureSlash();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.replaceSeprators();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeFileName();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeExtension();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeTrailingSlash();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());
}

TEST(Path, Construct2)
{
	core::Path<char> path;

}

TEST(Path, ConstructW)
{
	core::Path<wchar_t> path;

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	EXPECT_EQ(260, path.capacity());
	EXPECT_EQ(0, path.length());

	path.ensureSlash();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.replaceSeprators();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeFileName();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeExtension();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());

	path.removeTrailingSlash();

	EXPECT_TRUE(path.isEmpty());
	EXPECT_FALSE(path.isNotEmpty());
	EXPECT_FALSE(path.isAbsolute());
}