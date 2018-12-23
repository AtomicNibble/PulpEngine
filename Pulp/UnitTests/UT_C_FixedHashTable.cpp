#include "stdafx.h"

#include <Containers\FixedHashTable.h>

X_USING_NAMESPACE;

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
    

    template<size_t N>
    using HashTableIntIntN = core::FixedHashTableStack<N, int, int>;

    using HashTableIntInt16 = HashTableIntIntN<16>;

    using HashTableIntInt16StrKey = core::FixedHashTableStack<16, int, int, HashIntStr, EqualIntStr>;

} // namespace


TEST(FixedHashTable, MoveConstruct)
{
    HashTableIntInt16 ht0;
    ht0[1] = 1;

    HashTableIntInt16 ht1(std::move(ht0));

    EXPECT_TRUE(ht1.isNotEmpty());
    EXPECT_EQ(1, ht1.size());
    EXPECT_EQ(1, ht1[1]);
}

TEST(FixedHashTable, Iterator)
{
    HashTableIntIntN<128> ht;
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
        ht[i] = i;
    }

    std::array<bool, 100> visited = {};
    for (auto it = ht.begin(); it != ht.end(); ++it) {
        visited[it->first] = true;
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


TEST(FixedHashTable, Capacity)
{
    HashTableIntInt16 ht;

    const auto &cht = ht;
    EXPECT_FALSE(cht.isNotEmpty());
    EXPECT_TRUE(cht.isEmpty());
    EXPECT_EQ(0, cht.size());
    ht[1] = 1;
    EXPECT_TRUE(cht.isNotEmpty());
    EXPECT_FALSE(cht.isEmpty());
}

TEST(FixedHashTable, Clear)
{
    HashTableIntInt16 ht;

    ht[1] = 1;
    ht.clear();
    EXPECT_TRUE(ht.isEmpty());
    EXPECT_TRUE(ht.size() == 0);
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TEST(FixedHashTable, Insert)
{
    HashTableIntInt16 ht;

    auto res = ht.insert({ 1, 1 }); // xvalue

    EXPECT_FALSE(ht.isEmpty());

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_TRUE(res.first != ht.end());
    EXPECT_EQ(1, res.first->first);
    EXPECT_EQ(1, res.first->second);
    EXPECT_TRUE(res.second);

    const auto v = std::make_pair(1, 2);
    auto res2 = ht.insert(v); // rvalue

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(res2.first == res.first);
    EXPECT_EQ(1, res2.first->first);
    EXPECT_EQ(1, res2.first->second);
    EXPECT_FALSE(res2.second);
}

TEST(FixedHashTable, Emplace)
{
    HashTableIntInt16 ht;

    auto res = ht.emplace(1, 1); // xvalue
    EXPECT_FALSE(ht.isEmpty());
    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_TRUE(res.first != ht.end());
    EXPECT_EQ(1, res.first->first);
    EXPECT_EQ(1, res.first->second);
    EXPECT_TRUE(res.second);

    auto res2 = ht.emplace(1, 2);
    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(res2.first == res.first);
    EXPECT_EQ(1, res2.first->first);
    EXPECT_EQ(1, res2.first->second);
    EXPECT_FALSE(res2.second);
}

TEST(FixedHashTable, EraseIterator)
{
    HashTableIntInt16 ht;

    auto res = ht.emplace(1, 1);
    ht.erase(res.first);
    EXPECT_TRUE(ht.isEmpty());
    EXPECT_FALSE(ht.isNotEmpty());
    EXPECT_EQ(0, ht.size());
    EXPECT_TRUE(ht.begin() == ht.end());
    EXPECT_TRUE(ht.cbegin() == ht.cend());
}

TEST(FixedHashTable, EraseKey)
{
    HashTableIntInt16 ht;
    EXPECT_EQ(0, ht.erase(1));
    ht[1] = 1;
    EXPECT_EQ(1, ht.erase(1));

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

TEST(FixedHashTable, OperatorIndex)
{
    HashTableIntInt16 ht;
    ht[1] = 1;
    EXPECT_TRUE(ht.isNotEmpty());

    EXPECT_EQ(1, ht.size());
    EXPECT_TRUE(ht.begin() != ht.end());
    EXPECT_TRUE(ht.cbegin() != ht.cend());
    EXPECT_EQ(1, ht[1]);
}

TEST(FixedHashTable, Find)
{
    HashTableIntInt16 ht;

    const auto &cht = ht;
    ht[1] = 1;
    {
        auto it = ht.find(1);
        EXPECT_TRUE(it != ht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = ht.find(2);
        EXPECT_TRUE(it == ht.end());
    }
    {
        auto it = cht.find(1);
        EXPECT_TRUE(it != cht.end());
        EXPECT_EQ(1, it->first);
        EXPECT_EQ(1, it->second);
        it = cht.find(2);
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