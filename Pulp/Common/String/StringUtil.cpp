#include "EngineCommon.h"
#include "StringUtil.h"
#include "Path.h"

#include "StrRef.h"
#include <locale> // TODO: remove

#include <direct.h>

X_NAMESPACE_BEGIN(core)

namespace strUtil
{
    namespace
    {
#if X_COMPILER_CLANG
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#define MSBF16(x) bswap16((*(uint16_t const*)x))
#define MSBF32(x) bswap32((*(uint32_t const*)x))
#else
#define bswap16 _byteswap_ushort
#define bswap32 _byteswap_ulong
#define MSBF16(x) bswap16((*(uint16_t const __declspec(align(1))*)x))
#define MSBF32(x) bswap32((*(uint32_t const __declspec(align(1))*)x))
#endif

#define compxm(a, b) _mm_movemask_epi8(_mm_cmpeq_epi8((a), (b)))
#define xmload(p) _mm_load_si128((__m128i const*)(p))
#define load16(p) (*(uint16_t const*)(p))
#define load32(p) (*(uint32_t const*)(p))

        char const* scanstr2(char const* tgt, char const pat[2])
        {
            __m128i const zero = _mm_setzero_si128();
            __m128i const p0 = _mm_set1_epi8(pat[0]);
            __m128i const p1 = _mm_set1_epi8(pat[1]);
            unsigned f = 15 & (intptr_t)tgt;
            uint16_t pair = load16(pat);
            if (f) {
                __m128i x = xmload(tgt - f);
                unsigned u = compxm(zero, x) >> f;
                unsigned v = ((compxm(p0, x) & (compxm(p1, x) >> 1)) >> f) & ~u & (u - 1);
                if (v) {
                    return tgt + bitUtil::ScanBitsForward(v) - 1;
                }
                if (u) {
                    return nullptr;
                }
                tgt += 16 - f;
                if (load16(tgt - 1) == pair) {
                    return tgt - 1;
                }
            }

            X_DISABLE_WARNING(4127)
            while (true)
            X_ENABLE_WARNING(4127)
            {
                __m128i x = xmload(tgt);
                unsigned u = compxm(zero, x);
                unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1) & ~u & (u - 1);
                if (v) {
                    return tgt + bitUtil::ScanBitsForward(v);
                }
                if (u) {
                    return nullptr;
                }
                tgt += 16;
                if (load16(tgt - 1) == pair) {
                    return tgt - 1;
                }
            }
        }

        char const* scanstr3(char const* tgt, char const pat[3])
        {
            __m128i const zero = _mm_setzero_si128();
            __m128i const p0 = _mm_set1_epi8(pat[0]);
            __m128i const p1 = _mm_set1_epi8(pat[1]);
            __m128i const p2 = _mm_set1_epi8(pat[2]);
            unsigned trio = load32(pat) & 0x00FFFFFF;
            unsigned f = 15 & (uintptr_t)tgt;

            if (f) {
                __m128i x = xmload(tgt);
                unsigned u = compxm(zero, x);
                unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1);
                v = (v & (compxm(p2, x) >> 2) & ~u & (u - 1)) >> f;
                if (v) {
                    return tgt + bitUtil::ScanBitsForward(v) - 1;
                }
                tgt += 16 - f;
                v = load32(tgt - 2);
                if (trio == (v & 0x00FFFFFF)) {
                    return tgt - 2;
                }
                if (trio == v >> 8) {
                    return tgt - 1;
                }
                if (u >> f) {
                    return nullptr;
                }
            }

            X_DISABLE_WARNING(4127)
            while (true)
            X_ENABLE_WARNING(4127)
            {
                __m128i x = xmload(tgt);
                unsigned u = compxm(zero, x);
                unsigned v = compxm(p0, x) & (compxm(p1, x) >> 1);
                v = (v & (compxm(p2, x) >> 2) & ~u & (u - 1)) >> f;
                if (v) {
                    return tgt + bitUtil::ScanBitsForward(v) - 1;
                }
                tgt += 16;
                v = load32(tgt - 2);
                if (trio == (v & 0x00FFFFFF)) {
                    return tgt - 2;
                }
                if (trio == v >> 8) {
                    return tgt - 1;
                }
                if (u) {
                    return nullptr;
                }
            }
        }

        char const* scanstrm(char const* tgt, char const* pat, uint32_t len)
        {
            uint32_t head = MSBF32(pat), wind = 0, next;

            pat += 4, len -= 4;
            // warning C4706: assignment within conditional expression
            while ((next = *reinterpret_cast<uint8_t const*>(tgt++)) != 0) {
                wind = (wind << 8) + next;
                if (wind == head && !memcmp(tgt, pat, len))
                    return tgt - 4;
            }
            return nullptr;
        }

        // only used for function below.
        uint32_t replaceAll(char* startInclusive, char* endExclusive, const char original, const char replacement)
        {
            // find me baby
            uint32_t count = 0;
            char* start = (char*)startInclusive;
            while (start < endExclusive) {
                if (*start == original) {
                    *start = replacement;
                    count++;
                }
                ++start;
            }
            return count;
        }

        void ReplaceSlashes(char* path, char* pathEnd)
        {
            replaceAll(path, pathEnd, Path<char>::INVALID_SLASH, Path<char>::SLASH);
        }

#if X_COMPILER_MSVC

        template<size_t N>
        struct Implementation
        {
        };

        template<>
        struct Implementation<8u>
        {
            static size_t strlen(const char* str)
            {
                __m128i zero = _mm_set1_epi8(0);
                __m128i* s_aligned = (__m128i*)(((uint64_t)str) & -0x10L);
                uint8_t misbits = (uint8_t)(((uint64_t)str) & 0xf);
                __m128i s16cs = _mm_load_si128(s_aligned);
                __m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
                int bitmask = _mm_movemask_epi8(bytemask);
                bitmask = (bitmask >> misbits) << misbits;

                // Alternative: use TEST instead of BSF, then BSF at end (only). Much better on older CPUs
                // TEST has latency 1, while BSF has 3!
                while (bitmask == 0) {
                    s16cs = _mm_load_si128(++s_aligned);
                    bytemask = _mm_cmpeq_epi8(s16cs, zero);
                    bitmask = _mm_movemask_epi8(bytemask);
                }

                return (((const char*)s_aligned) - str) + (size_t)bitUtil::ScanBitsForward(bitmask);
            }
        };

        template<>
        struct Implementation<4u>
        {
#if X_64 == 0
            static size_t strlen(const char* str)
            {
                __m128i zero = _mm_set1_epi8(0);
                __m128i* s_aligned = (__m128i*)(((long)str) & -0x10L);
                uint8_t misbits = (uint8_t)(((long)str) & 0xf);
                __m128i s16cs = _mm_load_si128(s_aligned);
                __m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
                int bitmask = _mm_movemask_epi8(bytemask);
                bitmask = (bitmask >> misbits) << misbits;

                while (bitmask == 0) {
                    s16cs = _mm_load_si128(++s_aligned);
                    bytemask = _mm_cmpeq_epi8(s16cs, zero);
                    bitmask = _mm_movemask_epi8(bytemask);
                }
                return (((const char*)s_aligned) - str) + bitUtil::ScanBitsForward(bitmask);
            }
#endif
        };

#endif // !X_COMPILER_CLANG

        uint32_t upperCaseSIMD(uint32_t x)
        {
            uint32_t b = 0x80808080ul | x;
            uint32_t c = b - 0x61616161ul;
            uint32_t d = ~(b - 0x7b7b7b7bul);
            uint32_t e = (c & d) & (~x & 0x80808080ul);
            return x - (e >> 2);
        }

        void toLowerAscii8(char& c)
        {
            c = ToLower(c);
        }

        void toLowerAscii32(uint32_t& c) 
        {
            uint32_t rotated = c & uint32_t(0x7f7f7f7fL);
            rotated += uint32_t(0x25252525L);
            rotated &= uint32_t(0x7f7f7f7fL);
            rotated += uint32_t(0x1a1a1a1aL);

            rotated &= ~c;
            rotated >>= 2;
            rotated &= uint32_t(0x20202020L);
            c += rotated;
        }

        void toLowerAscii64(uint64_t& c) 
        {
            uint64_t rotated = c & uint64_t(0x7f7f7f7f7f7f7f7fL);
            rotated += uint64_t(0x2525252525252525L);
            rotated &= uint64_t(0x7f7f7f7f7f7f7f7fL);
            rotated += uint64_t(0x1a1a1a1a1a1a1a1aL);
            rotated &= ~c;
            rotated >>= 2;
            rotated &= uint64_t(0x2020202020202020L);
            c += rotated;
        }

    } // namespace

    void ToLower(char* startInclusive, char* endExclusive)
    {
        size_t len = endExclusive - startInclusive;

        constexpr size_t AlignMask64 = 7;
        constexpr size_t AlignMask32 = 3;

        size_t n = reinterpret_cast<size_t>(startInclusive);
        n &= AlignMask32;
        n = std::min(n, len);

        size_t offset = 0;
        if (n != 0) {
            n = std::min(4 - n, len);
            do {
                toLowerAscii8(startInclusive[offset]);
                offset++;
            } while (offset < n);
        }

        n = reinterpret_cast<size_t>(startInclusive + offset);
        n &= AlignMask64;
        if ((n != 0) && (offset + 4 <= len)) {
            toLowerAscii32(*(uint32_t*)(startInclusive + offset));
            offset += 4;
        }

        // Convert 8 characters at a time
        while (offset + 8 <= len) {
            toLowerAscii64(*(uint64_t*)(startInclusive + offset));
            offset += 8;
        }

        // Convert 4 characters at a time
        while (offset + 4 <= len) {
            toLowerAscii32(*(uint32_t*)(startInclusive + offset));
            offset += 4;
        }

        // Convert any characters remaining after the last 4-byte aligned group
        while (offset < len) {
            toLowerAscii8(startInclusive[offset]);
            offset++;
        }
    }

    void ToUpper(char* startInclusive, char* endExclusive)
    {
        size_t len = endExclusive - startInclusive;

        for (size_t i = 0; i < len; i++) {
            startInclusive[i] = ToUpper(startInclusive[i]);
        }
    }

    bool IsLower(const char character)
    {
        std::locale loc;
        return std::islower(character, loc);
    }

    bool IsLower(const char* startInclusive)
    {
        return IsLower(startInclusive, startInclusive + strlen(startInclusive));
    }

    bool IsLower(const char* startInclusive, const char* endExclusive)
    {
        std::locale loc;

        // return true if empty.
        size_t Len = endExclusive - startInclusive;
        if (Len == 0) {
            return true;
        }

        auto it = std::find_if(startInclusive, endExclusive, [&](char c) {
            return std::isupper(c, loc);
        });

        return it == endExclusive;
    }

    size_t strlen(const char* str)
    {
#if X_COMPILER_CLANG
        return std::strlen(str);
#else
        return static_cast<size_t>(Implementation<sizeof(const char*)>::strlen(str));
#endif // !X_COMPILER_CLANG
    }

    size_t strlen(const wchar_t* str)
    {
        return static_cast<size_t>(wcslen(str));
    }

    const char* Convert(const wchar_t* startInclusive, const wchar_t* endExclusive, char* output, size_t outputBytes, size_t& lengthOut)
    {
        if (!outputBytes) {
            return output;
        }

        const int32_t srcLenChars = safe_static_cast<int32_t>(endExclusive - startInclusive);
        const int32_t bytesWritten = WideCharToMultiByte(
            CP_UTF8,
            0,
            startInclusive,
            srcLenChars,
            output,
            safe_static_cast<int32_t>(outputBytes - 1), // space for nullterm.
            nullptr,
            nullptr
        );

        if (bytesWritten == 0)
        {
            // error
            lengthOut = 0;
        }
        else
        {
            lengthOut = bytesWritten;
        }

        output[lengthOut] = '\0';
        return output;
    }

    const wchar_t* Convert(const char* startInclusive, const char* endExclusive, wchar_t* output, size_t outputBytes, size_t& lengthOut)
    {
        if (!outputBytes) {
            return output;
        }

        const int32_t srcLenBytes = safe_static_cast<int32_t>(endExclusive - startInclusive);
        const int32_t charsWritten = MultiByteToWideChar(
            CP_UTF8, 
            0, 
            startInclusive, 
            srcLenBytes, 
            output, 
            safe_static_cast<int32_t>((outputBytes - 1) / sizeof(wchar_t)) // space for nullterm.
        );

        if (charsWritten == 0)
        {
            // error
            lengthOut = 0;
        }
        else
        {
            lengthOut = charsWritten;
        }

        output[lengthOut] = L'\0';
        return output;
    }

    unsigned int Count(const char* startInclusive, const char* endExclusive, char what)
    {
        uint i;
        const char* Start = startInclusive;

        for (i = 0; Start < endExclusive; ++Start) {
            if (*Start == what) {
                ++i;
            }
        }

        return i;
    }

    int Compare(const char* startInclusiveS1, const char* endExclusiveS1, 
        const char* startInclusiveS2, const char* endExclusiveS2)
    {
        size_t len1 = endExclusiveS1 - startInclusiveS1;
        size_t len2 = endExclusiveS2 - startInclusiveS2;

        auto shortest = core::Min(len1, len2);

        auto res = std::memcmp(startInclusiveS1, startInclusiveS2, shortest);

        if (res != 0) {
            return res;
        }

        if (len1 < len2) {
            return -1;
        }

        if (len1 > len2) {
            return 1;
        }

        return 0;
    }

    bool IsEqual(const char* str1, const char* str2)
    {
        X_ASSERT_NOT_NULL(str1);
        X_ASSERT_NOT_NULL(str2);

        while (*str1 && (*str1 == *str2)) {
            str1++, str2++;
        }

        // are they both null ?
        return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
    }

    // Returns whether two strings are equal, checks the length of both srings are equal.
    bool IsEqual(core::string_view str1, core::string_view str2)
    {
        return IsEqual(str1.begin(), str1.end(), str2.begin(), str2.end());
    }

    bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2)
    {
        size_t Len = endExclusiveS1 - startInclusiveS1;
        size_t Len2 = strlen(startInclusiveS2);

        if (Len == Len2) {
            return memcmp(startInclusiveS1, startInclusiveS2, Len) == 0;
        }

        return false;
    }

    bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2)
    {
        ptrdiff_t Len = endExclusiveS1 - startInclusiveS1;

        if (Len == (endExclusiveS2 - startInclusiveS2)) {
            return memcmp(startInclusiveS1, startInclusiveS2, Len) == 0;
        }

        return false;
    }

    bool IsEqualCaseInsen(core::string_view str1, core::string_view str2)
    {
        return IsEqualCaseInsen(str1.begin(), str1.end(), str2.begin(), str2.end());
    }

    bool IsEqualCaseInsen(const char* str1, const char* str2)
    {
        X_ASSERT_NOT_NULL(str1);
        X_ASSERT_NOT_NULL(str2);

        while (*str1 && (::tolower(*str1) == ::tolower(*str2))) {
            str1++, str2++;
        }

        // are they both null ?
        return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
    }

    // Returns whether two strings are equal, checks the length of the 1st range.
    bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2)
    {
        ptrdiff_t Len = endExclusiveS1 - startInclusiveS1;

        while (Len && *startInclusiveS2
               && (::tolower(*startInclusiveS1) == ::tolower(*startInclusiveS2))) {
            Len--;
            startInclusiveS1++;
            startInclusiveS2++;
        }

        return Len == 0 && !(*startInclusiveS2);
    }

    bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2)
    {
        ptrdiff_t Len = endExclusiveS1 - startInclusiveS1;

        if (Len == (endExclusiveS2 - startInclusiveS2)) {
            // need to respect length values.
            while (Len && (::tolower(*startInclusiveS1) == ::tolower(*startInclusiveS2))) {
                Len--;
                startInclusiveS1++;
                startInclusiveS2++;
            }
            return Len == 0;
        }

        return false;
    }

    // Finds the first character in a string that is not a certain character, and returns a pointer to it.
    // Returns a \c nullptr if the character could not be found.
    const char* FindNon(const char* startInclusive, const char* endExclusive, char what)
    {
        if (startInclusive >= endExclusive) {
            return nullptr;
        }

        const char* result = startInclusive;

        while (*result == what) {
            ++result;
            if (result >= endExclusive) {
                return nullptr;
            }
        }

        return result;
    }

    // Finds a character in a string, and returns a pointer to the last occurrence of the character.
    // Returns a \c nullptr if the character could not be found.
    const char* FindLast(const char* startInclusive, const char* endExclusive, char what)
    {
        if (startInclusive >= endExclusive) {
            return nullptr;
        }

        const char* result = endExclusive - 1;

        while (*result != what) {
            if (result == startInclusive) {
                return nullptr;
            }
            --result;
        }

        return result;
    }

    const char* FindLast(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd)
    {
        if (whatStart >= whatEnd) {
            return nullptr;
        }

        const ptrdiff_t whatLen = whatEnd - whatStart;

        if (startInclusive + whatLen > endExclusive) {
            return nullptr;
        }

        const char* pResult = endExclusive - whatLen;

        while (!IsEqual(pResult, pResult + whatLen, whatStart, whatEnd))
        {
            if (pResult == startInclusive) {
                return nullptr;
            }
            --pResult;
        }

        return pResult;
    }

    // Finds the last character in a string that is not a certain character, and returns a pointer to it.
    // Returns a \c nullptr if the character could not be found.
    const char* FindLastNon(const char* startInclusive, const char* endExclusive, char what)
    {
        const char* result = endExclusive - 1;

        if (startInclusive >= result) {
            return nullptr;
        }

        while (*result == what) {
            --result;
            if (startInclusive >= result) {
                return nullptr;
            }
        }

        return result;
    }

    // Finds the first whitespace character in a string, and returns a pointer to it.
    // Returns a \c nullptr if no such character could not be found.
    const char* FindWhitespace(const char* startInclusive, const char* endExclusive)
    {
        while (startInclusive < endExclusive) {
            if (*startInclusive == ' ') {
                return startInclusive;
            }
            ++startInclusive;
        }

        return nullptr;
    }

    const char* FindLastWhitespace(const char* startInclusive, const char* endExclusive)
    {
        do {
            --endExclusive;
            // reached the end ?
            if (endExclusive < startInclusive) {
                break;
            }
            // if whitespace return it.
            if (IsWhitespace(*endExclusive)) {
                return endExclusive;
            }
        }
        X_DISABLE_WARNING(4127)
        while (1)
            ;
        X_ENABLE_WARNING(4127)

        return nullptr;
    }

    const char* FindLastNonWhitespace(const char* startInclusive, const char* endExclusive)
    {
        do {
            --endExclusive;
            // reached the end ?
            if (endExclusive < startInclusive) {
                break;
            }
            // if not whitespace return it.
            if (!IsWhitespace(*endExclusive)) {
                return endExclusive;
            }
        }
        X_DISABLE_WARNING(4127)
        while (1)
            ;
        X_ENABLE_WARNING(4127)

        return nullptr;
    }

    const char* FindNonWhitespace(const char* startInclusive, const char* endExclusive)
    {
        while (startInclusive < endExclusive) {
            // if not whitespace return it.
            if (!IsWhitespace(*startInclusive)) {
                return startInclusive;
            }

            ++startInclusive;
        }

        return nullptr;
    }

    const char* Find(const char* startInclusive, char what)
    {
        return Find(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    // Finds a character in a string, and returns a pointer to the first occurrence of the character.
    // Returns a \c nullptr if the character could not be found.
    const char* Find(const char* startInclusive, const char* endExclusive, char what)
    {
        while (startInclusive < endExclusive) {
            if (*startInclusive == what) {
                return startInclusive;
            }
            ++startInclusive;
        }

        return nullptr;
    }

    const char* Find(const char* startInclusive, const char* what)
    {
        return Find(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    const char* Find(const char* startInclusive, const char* endExclusive, const char* what)
    {
        return Find(startInclusive, endExclusive, what, strlen(what));
    }

    const char* Find(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength)
    {
        if (whatLength == 0) {
            return startInclusive;
        }

#if 0 
		// should check how much these aligne ment checks take.
		// might be best to have a seprate aligned find
		// for when i know it's aligned, as the global optermisation attempt.
		// may be slower due to align checks.
		if (core::pointerUtil::IsAligned(startInclusive, 16, 0) &&
			core::pointerUtil::IsAligned(what, 16, 0))
		{

			switch (whatLength) {
				case  0: return startInclusive;
				case  1: return strchr(startInclusive, *what);
				case  2: return scanstr2(startInclusive, what);
				case  3: return scanstr3(startInclusive, what);
				default:
					return scanstrm(startInclusive, what, whatLength);
			}
		}
#endif
        size_t len = endExclusive - startInclusive;

        if (whatLength > len) {
            return nullptr;
        }

        while (startInclusive < endExclusive) {
            // we check if we have a match.
            // we need to match the whole string.
            if (*startInclusive == *what) {
                // we know the substring fits.
                // so we just want to know if we have the rest of the substring.
                const char* current = startInclusive + 1;
                const char* currentWhat = what + 1;
                size_t i = 0;
                for (; i < (whatLength - 1); i++) {
                    if (*current != *currentWhat) {
                        break;
                    }

                    ++current;
                    ++currentWhat;
                }

                if (i == (whatLength - 1)) {
                    return startInclusive;
                }
            }

            if (len == whatLength) { // sub string can't fit anymore.
                return nullptr;
            }

            --len;
            ++startInclusive;
        }

        return nullptr;
    }

    const char* Find(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd)
    {
        return Find(startInclusive, endExclusive, whatStart, safe_static_cast<size_t>(whatEnd - whatStart));
    }

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what)
    {
        return FindCaseInsensitive(startInclusive, endExclusive, what, strlen(what));
    }

    const char* FindCaseInsensitive(const char* startInclusive, const char* what)
    {
        return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what, strlen(what));
    }

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd)
    {
        return FindCaseInsensitive(startInclusive, endExclusive, whatStart, safe_static_cast<size_t>(whatEnd - whatStart));
    }

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength)
    {
        size_t len = endExclusive - startInclusive;

        if (whatLength > len) {
            return nullptr;
        }

        while (startInclusive < endExclusive) {
            // we check if we have a match.
            // we need to match the whole string.
            if (upperCaseSIMD(*startInclusive) == upperCaseSIMD(*what)) {
                // we know the substring fits.
                // so we just want to know if we have the rest of the substring.
                const char* current = startInclusive + 1;
                const char* currentWhat = what + 1;
                size_t i = 0;
                for (; i < (whatLength - 1); i++) {
                    if (upperCaseSIMD(*current) != upperCaseSIMD(*currentWhat)) {
                        break;
                    }

                    ++current;
                    ++currentWhat;
                }

                if (i == (whatLength - 1)) {
                    return startInclusive;
                }
            }

            if (len == whatLength) { // sub string can't fit anymore.
                return nullptr;
            }

            --len;
            ++startInclusive;
        }

        return nullptr;
    }

    const char* FindCaseInsensitive(const char* startInclusive, char what)
    {
        return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, char what)
    {
        size_t len = endExclusive - startInclusive;

        if (len == 0) {
            return nullptr;
        }

        uint32_t upperWhat = upperCaseSIMD(what);

        while (startInclusive < endExclusive) {
            if (upperCaseSIMD(*startInclusive) == upperWhat) {
                return startInclusive;
            }

            ++startInclusive;
        }

        return nullptr;
    }

    float StringToFloat(const char* str)
    {
        return StringToFloat(str, str + strlen(str));
    }

    float StringToFloat(const char* str, const char** pEndPtr)
    {
        return StringToFloat(str, str + strlen(str), pEndPtr);
    }

    float StringToFloat(const char* startInclusive, const char* endExclusive)
    {
        float val;
        auto res = std::from_chars(startInclusive, endExclusive, val, std::chars_format::general);
        return val;
    }

    float StringToFloat(const char* startInclusive, const char* endExclusive, const char** pEndPtr)
    {
        float val;
        auto res = std::from_chars(startInclusive, endExclusive, val, std::chars_format::general);
        *pEndPtr = res.ptr;
        return val;
    }

    // we support numeric and:'true', 'false'
    bool StringToBool(const char* str)
    {
        if (IsNumeric(str)) {
            int32_t val = StringToInt<int32_t>(str);
            // anything not zero is true.
            return val != 0;
        }

        if (core::strUtil::IsEqualCaseInsen(str, "true")) {
            return true;
        }

        return false;
    }

    bool StringToBool(const char* startInclusive, const char* endExclusive)
    {
        if (IsNumeric(startInclusive, endExclusive)) {
            int32_t val = StringToInt<int32_t>(startInclusive, endExclusive);
            // anything not zero is true.
            return val != 0;
        }

        if (core::strUtil::IsEqualCaseInsen(startInclusive, endExclusive, "true")) {
            return true;
        }

        return false;
    }

    bool HasFileExtension(const char* path)
    {
        return FileExtension(path) != nullptr;
    }

    bool HasFileExtension(const char* startInclusive, const char* endExclusive)
    {
        return FileExtension(startInclusive, endExclusive) != nullptr;
    }

    const char* FileExtension(const char* path)
    {
        return FileExtension(path, path + strUtil::strlen(path));
    }
    const char* FileExtension(const char* startInclusive, const char* endExclusive)
    {
        const char* res = strUtil::FindLast(startInclusive, endExclusive, '.');
        // I think it might be better to return null here, instead of start.
        if (!res || res == (endExclusive - 1)) {
            return nullptr;
        }
        return res + 1;
    }

    const char* FileName(const char* path)
    {
        return FileName(path, path + strUtil::strlen(path));
    }

    const char* FileName(const char* startInclusive, const char* endExclusive)
    {
        // make sure slash is correct.
        X_ASSERT(Find(startInclusive, endExclusive, Path<>::INVALID_SLASH) == nullptr, "Path Invalid slash")();

        const char* res = strUtil::FindLast(startInclusive, endExclusive, Path<char>::SLASH);

        if (!res || res == (endExclusive - 1)) {
            return startInclusive; // might just be file name.
        }
        return res + 1;
    }

    bool WildCompare(core::string_view wild, core::string_view string)
    {
        // empty wild don't match anything.
        if (wild.empty()) {
            return false;
        }

        auto* pWild = wild.begin();
        auto* pWildEnd = wild.end();

        auto* pStr = string.begin();
        auto* pStrEnd = string.end();

        // move forward :D
        while (pStr < pStrEnd && pWild < pWildEnd && *pWild != '*')
        {
            if (*pWild != *pStr && *pWild != '?') {
                return false;
            }

            pStr++;
            pWild++;
        }

        const char *cp = nullptr, *mp = nullptr;

        while (pStr < pStrEnd)
        {
            if (*pWild == '*') 
            {
                if (++pWild == pWildEnd) {
                    return true;
                }

                mp = pWild;
                cp = pStr + 1;
            }
            else if (*pWild == *pStr || *pWild == '?')
            {
                pWild++;
                pStr++;
            }
            else 
            {
                pWild = mp;
                pStr = cp++;
            }
        }

        while (pWild < pWildEnd && *pWild == '*') {
            pWild++;
        }

        // end of wild?
        return pWild == pWildEnd;
    }

    bool WildCompare(const char* wild, const char* string)
    {
        const char *cp = nullptr, *mp = nullptr;

        while ((*string) && (*wild != '*')) {
            if ((*wild != *string) && (*wild != '?')) {
                return 0;
            }
            wild++;
            string++;
        }

        while (*string) {
            if (*wild == '*') {
                if (!*++wild) {
                    return 1;
                }
                mp = wild;
                cp = string + 1;
            }
            else if ((*wild == *string) || (*wild == '?')) {
                wild++;
                string++;
            }
            else {
                wild = mp;
                string = cp++;
            }
        }

        while (*wild == '*') {
            wild++;
        }
        return !*wild;
    }

    size_t LineNumberForOffset(const char* pBegin, const char* pEnd, size_t offset)
    {
        X_ASSERT(pBegin <= pEnd, "Invalid range")(pBegin, pEnd);

        size_t size = (pEnd - pBegin);
        if (offset > size) {
            X_WARNING("", "Offset out of range. size %" PRIuS " offset: %" PRIuS, size, offset);
            return 1;
        }

        size_t line = 1; // :| !
        for (size_t i = 0; i < offset; i++) {
            line += (pBegin[i] == '\n');
        }

        return line;
    }

    size_t NumLines(const char* pBegin, const char* pEnd)
    {
        X_ASSERT(pBegin <= pEnd, "Invalid range")(pBegin, pEnd);

        size_t lines = 1; // :| !
        size_t length = pEnd - pBegin;
        for (size_t i = 0; i < length; i++) {
            lines += (pBegin[i] == '\n');
        }

        return lines;
    }

} // namespace strUtil

X_NAMESPACE_END
