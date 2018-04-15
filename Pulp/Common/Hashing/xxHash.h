#pragma once

#include <Util\Span.h>

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    // Version	Speed on 64-bits	Speed on 32-bits
    // XXH64	13.8 GB / s			1.9 GB / s
    // XXH32	6.8 GB / s			6.0 GB / s

    typedef uint32_t xxHash32Val;
    typedef uint64_t xxHash64Val;

    class xxHash32
    {
    public:
        typedef uint32_t HashVal;

    public:
        xxHash32();
        ~xxHash32() = default;

        void reset(uint32_t seed = 0);
        bool updateBytes(const void* pBuf, size_t bytelength);

        template<typename T>
        X_INLINE bool update(span<T> data);
        template<typename T>
        X_INLINE bool update(const T& type);

        HashVal finalize(void);

        static HashVal calc(const void* pInput, size_t bytelength, uint32_t seed = 0);

    private:
        struct
        {
            long long ll[6];
        } state_;
    };

    class xxHash64
    {
    public:
        typedef uint64_t HashVal;

    public:
        xxHash64();
        ~xxHash64() = default;

        void reset(uint64_t seed = 0);    
        bool updateBytes(const void* pBuf, size_t bytelength);

        template<typename T>
        X_INLINE bool update(span<T> data);
        template<typename T>
        X_INLINE bool update(const T& type);

        HashVal finalize(void);

        static HashVal calc(const void* pInput, size_t bytelength, uint64_t seed = 0);

    private:
        struct
        {
            long long ll[11];
        } state_;
    };

} // namespace Hash

X_NAMESPACE_END

#include "xxHash.inl"