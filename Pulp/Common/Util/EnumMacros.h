#pragma once

#ifndef X_ENUM_MACRO_UTIL_H_
#define X_ENUM_MACRO_UTIL_H_

#include <Prepro\PreproNumArgs.h>
#include <Prepro\PreproStringize.h>
#include <Prepro\PreproExpandArgs.h>
#include <Prepro\PreproPassArgs.h>

#define X_DECLARE_ENUM_IMPL_ENUM(value, n) value = n,
#define X_DECLARE_ENUM_IMPL_TO_STRING(value, n) \
    case value:                                 \
        return X_PP_STRINGIZE(value);

#define X_DECLARE_ENUM_IMPL_HELPER(STRING_STR, ENUM_STR, ...)              \
    {                                                                      \
        static const unsigned int ENUM_COUNT = X_PP_NUM_ARGS(__VA_ARGS__); \
                                                                           \
        enum Enum                                                          \
        {                                                                  \
            ENUM_STR                                                       \
        };                                                                 \
                                                                           \
        static const char* ToString(uint32_t value)                        \
        {                                                                  \
            switch (value) {                                               \
                STRING_STR                                                 \
                default:                                                   \
                    X_NO_SWITCH_DEFAULT;                                   \
            }                                                              \
        }                                                                  \
    }

#define X_DECLARE_ENUM8_IMPL_HELPER(STRING_STR, ENUM_STR, ...)             \
    {                                                                      \
        static const unsigned int ENUM_COUNT = X_PP_NUM_ARGS(__VA_ARGS__); \
        enum Enum : uint8_t                                                \
        {                                                                  \
            ENUM_STR                                                       \
        };                                                                 \
        static const char* ToString(uint32_t value)                        \
        {                                                                  \
            switch (value) {                                               \
                STRING_STR                                                 \
                default:                                                   \
                    X_NO_SWITCH_DEFAULT;                                   \
            }                                                              \
        }                                                                  \
    }

#define X_DECLARE_ENUM_NUMERIC_LIMITS(enumType)                            \
namespace std                                                              \
{                                                                          \
    template<>                                                             \
    class numeric_limits<enumType::Enum>                                   \
    {                                                                      \
    public:                                                                \
        static constexpr size_t min() {                                   \
            return 0;                                                      \
        }                                                                  \
        static constexpr size_t max() {                                   \
            return enumType::ENUM_COUNT - 1;                               \
        }                                                                  \
                                                                           \
        static constexpr bool is_signed = numeric_limits<                  \
                                underlying_type<enumType::Enum>::type      \
                            >::is_signed;                                  \
    };                                                                     \
}


#define X_DECLARE_ENUM_IMPL(...) X_DECLARE_ENUM_IMPL_HELPER(X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_TO_STRING, __VA_ARGS__), X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_ENUM, __VA_ARGS__), __VA_ARGS__)
#define X_DECLARE_ENUM8_IMPL(...) X_DECLARE_ENUM8_IMPL_HELPER(X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_TO_STRING, __VA_ARGS__), X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_ENUM, __VA_ARGS__), __VA_ARGS__)

#define X_DECLARE_ENUM(name) struct name X_DECLARE_ENUM_IMPL
#define X_DECLARE_ENUM8(name) struct name X_DECLARE_ENUM8_IMPL

#endif // X_ENUM_MACRO_UTIL_H_