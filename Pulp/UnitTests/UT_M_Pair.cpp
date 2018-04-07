#include "stdafx.h"

#include <Math\XPair.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<float, double, int> MyTypes;
TYPED_TEST_CASE(Pairs, MyTypes);

template<typename T>
class Pairs : public ::testing::Test
{
public:
};

TYPED_TEST(Pairs, Genral)
{
    Pair<TypeParam> p(10, 10);
    Pair<TypeParam> p1(2, 6);

    p = p + p1;

    EXPECT_EQ(12, p.x);
    EXPECT_EQ(16, p.y);

    p.set(25, 20);

    EXPECT_EQ(25, p.x);
    EXPECT_EQ(20, p.y);
}