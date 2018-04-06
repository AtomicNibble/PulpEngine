#include "stdafx.h"

#include <Sorting\RadixSort.h>

X_USING_NAMESPACE;

using namespace core;

typedef ::testing::Types<uint8_t, uint16_t, uint32_t> MyTypes;
TYPED_TEST_CASE(RadixSort, MyTypes);

template<typename T>
class RadixSort : public ::testing::Test
{
public:
};

TYPED_TEST(RadixSort, index32_8bit)
{
    core::Array<uint8_t> vec(gEnv->pArena);
    core::Array<TypeParam> indexes(gEnv->pArena);

    // 4096 or the max of type if lower.
    const TypeParam num = static_cast<TypeParam>(core::Min<size_t>(4096,
        std::numeric_limits<TypeParam>::max()));

    vec.resize(num);
    for (auto& v : vec) {
        v = rand() % 0xff;
    }

    // create sorted indexes's.
    core::Sorting::radix_sort_buf(vec, indexes);

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), indexes.size());

    // validate the indexes give us sorted data.
    TypeParam lastValue = std::numeric_limits<TypeParam>::min();
    for (const auto idx : indexes) {
        const TypeParam val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }
}

TYPED_TEST(RadixSort, index32_16bit)
{
    core::Array<uint16_t> vec(gEnv->pArena);
    core::Array<TypeParam> indexes(gEnv->pArena);

    // 4096 or the max of type if lower.
    const TypeParam num = static_cast<TypeParam>(core::Min<size_t>(4096 * 4,
        std::numeric_limits<TypeParam>::max()));

    vec.resize(num);
    for (auto& v : vec) {
        v = rand() % 0xffff;
    }

    // create sorted indexes's.
    core::Sorting::radix_sort_buf(vec, indexes, gEnv->pArena);

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), indexes.size());

    // validate the indexes give us sorted data.
    uint16_t lastValue = std::numeric_limits<uint16_t>::min();
    for (const auto idx : indexes) {
        const auto val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }
}

TYPED_TEST(RadixSort, index32_32bit)
{
    core::Array<uint32_t> vec(gEnv->pArena);
    core::Array<TypeParam> indexes(gEnv->pArena);

    // 4096 or the max of type if lower.
    const TypeParam num = static_cast<TypeParam>(core::Min<size_t>(4096 * 4,
        std::numeric_limits<TypeParam>::max()));

    vec.resize(num);
    for (auto& v : vec) {
        v = ((rand() & 0xFFFF) << 16) | (rand() % 0xFFFF);
    }

    // create sorted indexes's.
    core::Sorting::radix_sort_buf(vec, indexes, gEnv->pArena);

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), indexes.size());

    // validate the indexes give us sorted data.
    uint32_t lastValue = std::numeric_limits<uint32_t>::min();
    for (const auto idx : indexes) {
        const auto val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }
}

TYPED_TEST(RadixSort, index32_64bit)
{
    core::Array<uint64_t> vec(gEnv->pArena);
    core::Array<TypeParam> indexes(gEnv->pArena);

    // 4096 or the max of type if lower.
    const TypeParam num = static_cast<TypeParam>(core::Min<size_t>(4096 * 8,
        std::numeric_limits<TypeParam>::max()));

    vec.resize(num);
    for (auto& v : vec) {
        v = (static_cast<uint64_t>(rand()) << 0) ^ (static_cast<uint64_t>(rand()) << 16) ^ (static_cast<uint64_t>(rand()) << 32) ^ (static_cast<uint64_t>(rand()) << 48);
    }

    // create sorted indexes's.
    core::Sorting::radix_sort_buf(vec, indexes, gEnv->pArena);

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), indexes.size());

    // validate the indexes give us sorted data.
    uint64_t lastValue = std::numeric_limits<uint64_t>::min();
    for (const auto idx : indexes) {
        const auto val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }
}

TEST(RadixSort, cachedSort)
{
    core::Array<uint32_t> vec(gEnv->pArena);

    // 4096 or the max of type if lower.
    const uint32_t num = static_cast<uint32_t>(core::Min<size_t>(4096 * 8,
        std::numeric_limits<uint32_t>::max()));

    vec.resize(num);
    for (auto& v : vec) {
        v = (static_cast<uint32_t>(rand()) << 0) ^ (static_cast<uint32_t>(rand()) << 16);
    }

    core::Sorting::RadixSort radix(g_arena);

    radix.sort(vec.data(), vec.size());

    auto sorted = radix.getIndexes();

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), sorted.second);

    // validate the indexes give us sorted data.
    uint32_t lastValue = std::numeric_limits<uint32_t>::min();
    for (size_t i = 0; i < sorted.second; i++) {
        const auto idx = sorted.first[i];
        const auto val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }

    radix.sort(vec.data(), vec.size());

    sorted = radix.getIndexes();

    // it should ensure the sizes match
    ASSERT_EQ(vec.size(), sorted.second);

    // validate the indexes give us sorted data.
    lastValue = std::numeric_limits<uint32_t>::min();
    for (size_t i = 0; i < sorted.second; i++) {
        const auto idx = sorted.first[i];
        const auto val = vec[idx];

        ASSERT_GE(val, lastValue);
        lastValue = val;
    }
}