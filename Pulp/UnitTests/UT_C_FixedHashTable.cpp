#include "stdafx.h"
#include "ComplexTypes.h"

#include <Containers\FixedHashTable.h>

X_USING_NAMESPACE;

using namespace testTypes;


namespace
{

    // These are used for checking we can use string key for int hash table.
    struct HashIntStr
    {
        size_t operator()(int v) { 
            return v * 7; 
        }
        size_t operator()(const std::string& v) { 
            return std::stoi(v) * 7; 
        }
    };

    struct EqualIntStr
    {
        bool operator()(int lhs, int rhs) { 
            return lhs == rhs; 
        }
        bool operator()(int lhs, const std::string &rhs) {
            return lhs == std::stoi(rhs);
        }
    };
    
    template<typename T>
    using HashTableT16 = core::FixedHashTableStack<16, T, T>;

    template<size_t N>
    using HashTableIntIntN = core::FixedHashTableStack<N, int, int>;

    using HashTableIntInt16 = HashTableIntIntN<16>;

    using HashTableIntInt16StrKey = core::FixedHashTableStack<16, int, int, HashIntStr, EqualIntStr>;

} // namespace

typedef ::testing::Types<short, int, float> MyTypes;
TYPED_TEST_CASE(FixedHashTable, MyTypes);

template<typename T>
class FixedHashTable : public ::testing::Test
{
public:
};

TEST(FixedHashTable, ComplexKey)
{
    resetConConters();

    using HashTable = core::FixedHashTableStack<16, CustomType, int>;

    {
        HashTable ht;

        ht.insert({ CustomType(16), 1 });
        ht.emplace(16, 1);
    }

    EXPECT_EQ(CONSRUCTION_COUNT, DECONSRUCTION_COUNT);
}

TEST(FixedHashTable, ComplexKeyValue)
{
    resetConConters();

    using HashTable = core::FixedHashTableStack<16, CustomType, CustomType>;

    {
        HashTable ht;

        ht.insert({ CustomType(16), CustomType(20) });
        ht.emplace(32, 64);
    }

    EXPECT_EQ(CONSRUCTION_COUNT, DECONSRUCTION_COUNT);
}


TYPED_TEST(FixedHashTable, MoveConstruct)
{
    HashTableT16<TypeParam> ht0;
    ht0[(TypeParam)1] = (TypeParam)1;

    HashTableT16<TypeParam> ht1(std::move(ht0));

    EXPECT_TRUE(ht1.isNotEmpty());
    EXPECT_EQ(1, ht1.size());
    EXPECT_EQ(1, ht1[1]);
}

TYPED_TEST(FixedHashTable, Iterator)
{
    using HashTable = core::FixedHashTableStack<128, TypeParam, TypeParam>;

    HashTable ht;
    const auto& cht = ht;

    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(cht.begin() == cht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
    EXPECT_TRUE(ht.cbegin() == cht.begin());
    EXPECT_TRUE(ht.cend() == cht.end());

    EXPECT_FALSE(ht.begin() != ht.end());
    EXPECT_FALSE(cht.begin() != cht.end());
    EXPECT_FALSE(ht.cbegin() != ht.cend());
    EXPECT_FALSE(ht.cbegin() != cht.begin());
    EXPECT_FALSE(ht.cend() != cht.end());

    const auto cit = ht.begin();
    EXPECT_TRUE(cit == ht.end());
    EXPECT_FALSE(cit != ht.end());

    for (int i = 1; i < 100; ++i) {
        ht[(TypeParam)i] = (TypeParam)i;
    }

    std::array<bool, 100> visited = {};
    for (auto it = ht.begin(); it != ht.end(); ++it) {
        visited[(int)it->first] = true;
    }

    for (int i = 1; i < 100; ++i) {
        EXPECT_TRUE(visited[i]);
    }

    // Test for iterator traits
    EXPECT_TRUE(std::all_of(ht.begin(), ht.end(), [](const auto& item) {
            return item.second > 0; 
        }
    ));
}

TYPED_TEST(FixedHashTable, Capacity)
{
    HashTableT16<TypeParam> ht;

    const auto &cht = ht;
    EXPECT_FALSE(cht.isNotEmpty());
    EXPECT_TRUE(cht.isEmpty());
    EXPECT_EQ(0, cht.size());
    ht[(TypeParam)1] = (TypeParam)1;
    EXPECT_TRUE(cht.isNotEmpty());
    EXPECT_FALSE(cht.isEmpty());
}

TYPED_TEST(FixedHashTable, Clear)
{
    HashTableT16<TypeParam> ht;

    ht[(TypeParam)1] = (TypeParam)1;
    ht.clear();
    EXPECT_TRUE(ht.isEmpty());
    EXPECT_TRUE(ht.size() == 0);
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TYPED_TEST(FixedHashTable, Insert)
{
    HashTableT16<TypeParam> ht;

    auto res = ht.insert({ (TypeParam)1, (TypeParam)1 }); // xvalue

    EXPECT_FALSE(ht.isEmpty());

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_TRUE(res.first != ht.end());
    EXPECT_EQ(1, res.first->first);
    EXPECT_EQ(1, res.first->second);
    EXPECT_TRUE(res.second);

    const auto v = std::make_pair((TypeParam)1, (TypeParam)2);
    auto res2 = ht.insert(v); // rvalue

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(res2.first == res.first);
    EXPECT_EQ(1, res2.first->first);
    EXPECT_EQ(1, res2.first->second);
    EXPECT_FALSE(res2.second);
}

TYPED_TEST(FixedHashTable, Emplace)
{
    HashTableT16<TypeParam> ht;

    auto res = ht.emplace((TypeParam)1, (TypeParam)1); // xvalue
    EXPECT_FALSE(ht.isEmpty());
    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_TRUE(res.first != ht.end());
    EXPECT_EQ(1, res.first->first);
    EXPECT_EQ(1, res.first->second);
    EXPECT_TRUE(res.second);

    auto res2 = ht.emplace((TypeParam)1, (TypeParam)2);
    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(res2.first == res.first);
    EXPECT_EQ(1, res2.first->first);
    EXPECT_EQ(1, res2.first->second);
    EXPECT_FALSE(res2.second);
}

TYPED_TEST(FixedHashTable, EraseIterator)
{
    HashTableT16<TypeParam> ht;

    auto res = ht.emplace((TypeParam)1, (TypeParam)1);
    ht.erase(res.first);
    EXPECT_TRUE(ht.isEmpty());
    EXPECT_FALSE(ht.isNotEmpty());
    EXPECT_EQ(0, ht.size());
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TYPED_TEST(FixedHashTable, EraseKey)
{
    HashTableT16<TypeParam> ht;
    EXPECT_EQ(0, ht.erase((TypeParam)1));
    ht[(TypeParam)1] = (TypeParam)1;
    EXPECT_EQ(1, ht.erase((TypeParam)1));

    EXPECT_TRUE(ht.isEmpty());
    EXPECT_FALSE(ht.isNotEmpty());
    EXPECT_EQ(0, ht.size());
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TEST(FixedHashTable, EraseKeyT)
{
    HashTableIntInt16StrKey ht;
    EXPECT_EQ(0, ht.erase("1"));
    ht[1] = 1;
    EXPECT_EQ(1, ht.erase("1"));

    EXPECT_TRUE(ht.isEmpty());
    EXPECT_FALSE(ht.isNotEmpty());
    EXPECT_EQ(0, ht.size());
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TYPED_TEST(FixedHashTable, OperatorIndex)
{
    HashTableT16<TypeParam> ht;
    ht[(TypeParam)1] = (TypeParam)1;
    EXPECT_TRUE(ht.isNotEmpty());

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_EQ(1, ht[1]);
}

TYPED_TEST(FixedHashTable, Find)
{
    HashTableT16<TypeParam> ht;

    const auto &cht = ht;
    ht[(TypeParam)1] = (TypeParam)1;
    {
        auto it = ht.find((TypeParam)1);
        EXPECT_TRUE(it != ht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = ht.find((TypeParam)2);
        EXPECT_TRUE(it == ht.end());
    }
    {
        auto it = cht.find((TypeParam)1);
        EXPECT_TRUE(it != cht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = cht.find((TypeParam)2);
        EXPECT_TRUE(it == cht.end());
    }
}

TEST(FixedHashTable, FindT)
{
    HashTableIntInt16StrKey ht;

    const auto &cht = ht;
    ht[1] = 1;
    {
        auto it = ht.find("1");
        EXPECT_TRUE(it != ht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = ht.find("2");
        EXPECT_TRUE(it == ht.end());
    }
    {
        auto it = cht.find("1");
        EXPECT_TRUE(it != cht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = cht.find("2");
        EXPECT_TRUE(it == cht.end());
    }
}