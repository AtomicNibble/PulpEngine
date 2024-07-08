#pragma once

#ifndef X_FLAGSMACROS_H_
#define X_FLAGSMACROS_H_

#define X_DECLARE_FLAGS_IMPL_ENUM(value, n) value = (1u << n),
#define X_DECLARE_FLAGS64_IMPL_ENUM(value, n) value = (1ull << n),
#define X_DECLARE_FLAGS_IMPL_BITS(member, n) uint32_t member : 1;
#define X_DECLARE_FLAGS64_IMPL_BITS(member, n) uint64_t member : 1;
#define X_DECLARE_FLAGS8_IMPL_BITS(member, n) uint8_t member : 1;
#define X_DECLARE_FLAGS_IMPL_TO_STRING(value, n) \
    case value:                                  \
        return X_PP_STRINGIZE(value);

#define X_DECLARE_FLAGS_IMPL(...)                                                                                             \
    {                                                                                                                         \
        static const unsigned int FLAGS_COUNT = X_PP_NUM_ARGS(__VA_ARGS__);                                                   \
        static_assert(FLAGS_COUNT <= 32, "Too many flags used in X_DECLARE_FLAGS. A maximum number of 32 flags is allowed."); \
                                                                                                                              \
        enum Enum                                                                                                             \
        {                                                                                                                     \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_ENUM, __VA_ARGS__)                                                          \
        };                                                                                                                    \
                                                                                                                              \
        struct Bits                                                                                                           \
        {                                                                                                                     \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_BITS, __VA_ARGS__)                                                          \
        };                                                                                                                    \
                                                                                                                              \
        static const char* ToString(uint32_t value)                                                                           \
        {                                                                                                                     \
            switch (value) {                                                                                                  \
                X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_TO_STRING, __VA_ARGS__)                                                 \
                default:                                                                                                      \
                    X_NO_SWITCH_DEFAULT;                                                                                      \
            }                                                                                                                 \
        }                                                                                                                     \
    }

#define X_DECLARE_FLAGS64_IMPL(...)                                                                                          \
    {                                                                                                                       \
        static const unsigned int FLAGS_COUNT = X_PP_NUM_ARGS(__VA_ARGS__);                                                 \
        static_assert(FLAGS_COUNT <= 64, "Too many flags used in X_DECLARE_FLAGS64_IMPL. A maximum number of 64 flags is allowed."); \
                                                                                                                            \
        enum Enum : uint64_t                                                                                                 \
        {                                                                                                                   \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS64_IMPL_ENUM, __VA_ARGS__)                                                        \
        };                                                                                                                  \
                                                                                                                            \
        struct Bits                                                                                                         \
        {                                                                                                                   \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS64_IMPL_BITS, __VA_ARGS__)                                                       \
        };                                                                                                                  \
                                                                                                                            \
        static const char* ToString(uint64_t value)                                                                          \
        {                                                                                                                   \
            switch (value) {                                                                                                \
                X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_TO_STRING, __VA_ARGS__)                                               \
                default:                                                                                                    \
                    X_NO_SWITCH_DEFAULT;                                                                                    \
            }                                                                                                               \
        }                                                                                                                   \
    }

#define X_DECLARE_FLAGS8_IMPL(...)                                                                                          \
    {                                                                                                                       \
        static const unsigned int FLAGS_COUNT = X_PP_NUM_ARGS(__VA_ARGS__);                                                 \
        static_assert(FLAGS_COUNT <= 8, "Too many flags used in X_DECLARE_FLAGS8. A maximum number of 8 flags is allowed."); \
                                                                                                                            \
        enum Enum : uint8_t                                                                                                 \
        {                                                                                                                   \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_ENUM, __VA_ARGS__)                                                        \
        };                                                                                                                  \
                                                                                                                            \
        struct Bits                                                                                                         \
        {                                                                                                                   \
            X_PP_EXPAND_ARGS(X_DECLARE_FLAGS8_IMPL_BITS, __VA_ARGS__)                                                       \
        };                                                                                                                  \
                                                                                                                            \
        static const char* ToString(uint8_t value)                                                                          \
        {                                                                                                                   \
            switch (value) {                                                                                                \
                X_PP_EXPAND_ARGS(X_DECLARE_FLAGS_IMPL_TO_STRING, __VA_ARGS__)                                               \
                default:                                                                                                    \
                    X_NO_SWITCH_DEFAULT;                                                                                    \
            }                                                                                                               \
        }                                                                                                                   \
    }

#define X_DECLARE_FLAGS(name) struct name X_DECLARE_FLAGS_IMPL
#define X_DECLARE_FLAGS64(name) struct name X_DECLARE_FLAGS64_IMPL
#define X_DECLARE_FLAGS8(name) struct name X_DECLARE_FLAGS8_IMPL

// can't seam to find a way to have this define by X_DECLARE_FLAGS
#define X_DECLARE_FLAG_OPERATORS(FlagsType)                            \
    X_INLINE constexpr FlagsType operator|(FlagsType::Enum a, FlagsType::Enum b) \
    {                                                                  \
        FlagsType r(a);                                                \
        r.Set(b);                                                      \
        return r;                                                      \
    }

#endif // X_FLAGSMACROS_H_
