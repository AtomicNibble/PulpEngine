#include "stdafx.h"

#include <Containers\HashMap.h>

X_USING_NAMESPACE;

using namespace core;
// using namespace std;

TEST(HashTable, PODTypes)
{
    typedef HashMap<int, bool> MapType;

    MapType hash(g_arena);

    EXPECT_EQ(0, hash.size());

    for (int i = 0; i < 80; i++) {
        hash.insert(MapType::value_type(i * 64, i % 2 == 0));
    }

    EXPECT_EQ(80, hash.size());

    // we should be abel to find them all
    for (int i = 0; i < 80; i++) {
        EXPECT_FALSE(hash.end() == hash.find(i * 64));
    }

    // it should not find any of these values
    for (int i = 0; i < 64; i++) {
        EXPECT_TRUE(hash.end() == hash.find((i + 1) * 8192));
    }

    hash.clear();
    EXPECT_EQ(0, hash.size());
    hash.free();
}

namespace
{
    X_ALIGNED_SYMBOL(struct UserType, 16)
    {
        UserType() :
            var_(0)
        {
            DEFAULT_CONSRUCTION_COUNT++;
        }

        explicit UserType(int val) :
            var_(val)
        {
            CONSRUCTION_COUNT++;
        }

        UserType(const UserType& oth) :
            var_(oth.var_)
        {
            ++CONSRUCTION_COUNT;
        }

        ~UserType()
        {
            DECONSRUCTION_COUNT++;
        }

        inline int GetVar() const
        {
            return var_;
        }

    private:
        int var_;

    public:
        static int DEFAULT_CONSRUCTION_COUNT;
        static int CONSRUCTION_COUNT;
        static int DECONSRUCTION_COUNT;
    };

    int UserType::DEFAULT_CONSRUCTION_COUNT = 0;
    int UserType::CONSRUCTION_COUNT = 0;
    int UserType::DECONSRUCTION_COUNT = 0;

} // namespace

TEST(HashTable, UserTypes)
{
    typedef HashMap<void*, UserType> MapType;

    MapType hash(g_arena);

    EXPECT_EQ(0, hash.size());

    int a = 0x99;
    int b = 0xff;
    int c = 0x1337;

    hash.insert(MapType::value_type(&a, UserType(a)));
    hash.insert(MapType::value_type(&b, UserType(b)));
    hash.insert(MapType::value_type(&c, UserType(c)));

    EXPECT_EQ(3, hash.size());

    ASSERT_TRUE(hash.contains(&a));
    ASSERT_TRUE(hash.contains(&b));
    ASSERT_TRUE(hash.contains(&c));

    EXPECT_EQ(a, hash[&a].GetVar());
    EXPECT_EQ(b, hash[&b].GetVar());
    EXPECT_EQ(c, hash[&c].GetVar());

    for (MapType::const_iterator it = hash.begin(); it != hash.end(); ++it) {
        switch (it->second.GetVar()) {
            case 0x99:
                EXPECT_EQ(&a, it->first);
                break;
            case 0xff:
                EXPECT_EQ(&b, it->first);
                break;
            case 0x1337:
                EXPECT_EQ(&c, it->first);
                break;
        }
    }

    hash.erase(&a);
    EXPECT_EQ(2, hash.size());
    EXPECT_FALSE(hash.contains(&a));
    hash.erase(0);
    EXPECT_EQ(2, hash.size());

    hash.erase(&b);
    hash.erase(&c);
    EXPECT_EQ(0, hash.size());

    EXPECT_FALSE(hash.contains(&a));
    EXPECT_FALSE(hash.contains(&b));
    EXPECT_FALSE(hash.contains(&c));

    hash.clear();
    EXPECT_EQ(0, hash.size());
    hash.free();

    EXPECT_EQ(0, UserType::DEFAULT_CONSRUCTION_COUNT);
    EXPECT_EQ(UserType::CONSRUCTION_COUNT, UserType::DECONSRUCTION_COUNT);
}

struct eqstr
{
    bool operator()(const char* s1, const char* s2) const
    {
        return strcmp(s1, s2) == 0;
    }
};

struct strhash
{
    size_t operator()(const char* s1) const
    {
        return Hash::Fnv1aHash(s1, strlen(s1));
    }
};

TEST(HashTable, String)
{
    typedef HashMap<const char*, int, strhash, eqstr> MapType;

    MapType hash(g_arena);

    EXPECT_EQ(0, hash.size());

    hash.insert(MapType::value_type("hello", 1));
    hash.insert(MapType::value_type("goat", 2));
    hash.insert(MapType::value_type("camel", 3));

    ASSERT_TRUE(hash.contains("hello"));
    ASSERT_TRUE(hash.contains("goat"));
    ASSERT_TRUE(hash.contains("camel"));

    EXPECT_EQ(1, hash["hello"]);
    EXPECT_EQ(2, hash["goat"]);
    EXPECT_EQ(3, hash["camel"]);

    hash.clear();
}