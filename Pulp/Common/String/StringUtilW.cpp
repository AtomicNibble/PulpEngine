#include "EngineCommon.h"
#include "StringUtil.h"
#include "Path.h"
#include "StrRef.h"

#include <direct.h>
#include <locale>

X_NAMESPACE_BEGIN(core)

namespace strUtil
{
    namespace
    {
        uint32_t upperCaseSIMD(uint32_t x)
        {
            uint32_t b = 0x80808080ul | x;
            uint32_t c = b - 0x61616161ul;
            uint32_t d = ~(b - 0x7b7b7b7bul);
            uint32_t e = (c & d) & (~x & 0x80808080ul);
            return x - (e >> 2);
        }

    } // namespace

    bool IsLowerW(const wchar_t character)
    {
        std::locale loc;
        return std::islower(character, loc);
    }

    bool IsLower(const wchar_t* startInclusive)
    {
        return IsLower(startInclusive, startInclusive + strlen(startInclusive));
    }

    bool IsLower(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        std::locale loc;

        size_t Len = endExclusive - startInclusive;
        if (Len == 0) {
            return true;
        }

        auto it = std::find_if(startInclusive, endExclusive, [&](wchar_t c) {
            return std::isupper(c, loc);
        });

        return it == endExclusive;
    }

    unsigned int Count(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        uint i;
        const wchar_t* Start = startInclusive;

        for (i = 0; Start < endExclusive; ++Start) {
            if (*Start == what)
                ++i;
        }

        return i;
    }

    int Compare(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1,
        const wchar_t* startInclusiveS2, const wchar_t* endExclusiveS2)
    {
        size_t len1 = endExclusiveS1 - startInclusiveS1;
        size_t len2 = endExclusiveS2 - startInclusiveS2;

        auto shortest = core::Min(len1, len2);

        auto res = std::wmemcmp(startInclusiveS1, startInclusiveS2, shortest);

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

    bool IsEqual(const wchar_t* str1, const wchar_t* str2)
    {
        while (*str1 && (*str1 == *str2))
            str1++, str2++;

        // are they both null ?
        return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
    }

    /// Returns whether two strings are equal, checks the length of both srings are equal.
    bool IsEqual(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2)
    {
        size_t Len = endExclusiveS1 - startInclusiveS1;
        size_t Len2 = strlen(startInclusiveS2);

        if (Len == Len2) {
            return memcmp(startInclusiveS1, startInclusiveS2, Len * sizeof(wchar_t)) == 0;
        }

        return false;
    }

    bool IsEqual(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2, const wchar_t* endExclusiveS2)
    {
        ptrdiff_t Len = endExclusiveS1 - startInclusiveS1;

        if (Len == (endExclusiveS2 - startInclusiveS2))
            return memcmp(startInclusiveS1, startInclusiveS2, Len * sizeof(wchar_t)) == 0;

        return false;
    }

    bool IsEqualCaseInsen(const wchar_t* str1, const wchar_t* str2)
    {
        while (*str1 && (::tolower(*str1) == ::tolower(*str2)))
            str1++, str2++;

        // are they both null ?
        return (*(const uint8_t*)str1 - *(const uint8_t*)str2) == 0;
    }

    /// Returns whether two strings are equal, checks the length of the 1st range.
    bool IsEqualCaseInsen(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2)
    {
        size_t Len = endExclusiveS1 - startInclusiveS1;

        while (Len && *startInclusiveS2
               && (::tolower(*startInclusiveS1) == ::tolower(*startInclusiveS2))) {
            Len--;
            startInclusiveS1++;
            startInclusiveS2++;
        }

        return Len == 0 && !(*startInclusiveS2);
    }

    bool IsEqualCaseInsen(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2, const wchar_t* endExclusiveS2)
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

    /// \brief Finds the first character in a string that is not a certain character, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the character could not be found.
    const wchar_t* FindNon(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        if (startInclusive >= endExclusive)
            return nullptr;

        const wchar_t* result = startInclusive;

        while (*result == what) {
            ++result;
            if (result >= endExclusive)
                return nullptr;
        }

        return result;
    }

    /// \brief Finds a character in a string, and returns a pointer to the last occurrence of the character.
    /// \remark Returns a \c nullptr if the character could not be found.
    const wchar_t* FindLast(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        const wchar_t* result = endExclusive - 1;

        if (startInclusive >= result)
            return nullptr;

        while (*result != what) {
            --result;
            if (startInclusive >= result)
                return nullptr;
        }

        return result;
    }

    /// \brief Finds the last character in a string that is not a certain character, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the character could not be found.
    const wchar_t* FindLastNon(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        const wchar_t* result = endExclusive - 1;

        if (startInclusive >= result)
            return nullptr;

        while (*result == what) {
            --result;
            if (startInclusive >= result)
                return nullptr;
        }

        return result;
    }

    /// \brief Finds the first whitespace character in a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if no such character could not be found.
    const wchar_t* FindWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        while (startInclusive < endExclusive) {
            if (*startInclusive == L' ')
                return startInclusive;
            ++startInclusive;
        }

        return nullptr;
    }

    const wchar_t* FindLastWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        do {
            --endExclusive;
            // reached the end ?
            if (endExclusive < startInclusive)
                break;
            // if whitespace return it.
            if (IsWhitespaceW(*endExclusive))
                return endExclusive;
        }
        X_DISABLE_WARNING(4127)
        while (1)
            ;
        X_ENABLE_WARNING(4127)

        return nullptr;
    }

    const wchar_t* FindLastNonWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        do {
            --endExclusive;
            // reached the end ?
            if (endExclusive < startInclusive)
                break;
            // if not whitespace return it.
            if (!IsWhitespaceW(*endExclusive))
                return endExclusive;
        }
        X_DISABLE_WARNING(4127)
        while (1)
            ;
        X_ENABLE_WARNING(4127)

        return nullptr;
    }

    const wchar_t* FindNonWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        while (startInclusive < endExclusive) {
            // if not whitespace return it.
            if (!IsWhitespaceW(*startInclusive))
                return startInclusive;

            ++startInclusive;
        }

        return nullptr;
    }

    const wchar_t* Find(const wchar_t* startInclusive, wchar_t what)
    {
        return Find(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    /// \brief Finds a character in a string, and returns a pointer to the first occurrence of the character.
    /// \remark Returns a \c nullptr if the character could not be found.
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        while (startInclusive < endExclusive) {
            if (*startInclusive == what)
                return startInclusive;
            ++startInclusive;
        }

        return nullptr;
    }

    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* what)
    {
        return Find(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what)
    {
        return Find(startInclusive, endExclusive, what, strlen(what));
    }

    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what, size_t whatLength)
    {
        size_t len = endExclusive - startInclusive;

        if (whatLength > len)
            return nullptr;

        while (startInclusive < endExclusive) {
            if (*startInclusive == *what) {
                const wchar_t* current = startInclusive + 1;
                const wchar_t* currentWhat = what + 1;
                size_t i = 0;
                for (; i < (whatLength - 1); i++) {
                    if (*current != *currentWhat)
                        break;

                    ++current;
                    ++currentWhat;
                }

                if (i == (whatLength - 1))
                    return startInclusive;
            }

            if (len == whatLength) // sub string can't fit anymore.
                return nullptr;

            --len;
            ++startInclusive;
        }

        return nullptr;
    }

    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* whatStart, const wchar_t* whatEnd)
    {
        return Find(startInclusive, endExclusive, whatStart, safe_static_cast<size_t>(whatEnd - whatStart));
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what)
    {
        return FindCaseInsensitive(startInclusive, endExclusive, what, strlen(what));
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* what)
    {
        return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what, strlen(what));
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* whatStart, const wchar_t* whatEnd)
    {
        return FindCaseInsensitive(startInclusive, endExclusive, whatStart, safe_static_cast<size_t>(whatEnd - whatStart));
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what, size_t whatLength)
    {
        size_t len = endExclusive - startInclusive;

        if (whatLength > len)
            return nullptr;

        while (startInclusive < endExclusive) {
            // we check if we have a match.
            // we need to match the whole string.
            if (upperCaseSIMD(*startInclusive) == upperCaseSIMD(*what)) {
                // we know the substring fits.
                // so we just want to know if we have the rest of the substring.
                const wchar_t* current = startInclusive + 1;
                const wchar_t* currentWhat = what + 1;
                size_t i = 0;
                for (; i < (whatLength - 1); i++) {
                    if (upperCaseSIMD(*current) != upperCaseSIMD(*currentWhat))
                        break;

                    ++current;
                    ++currentWhat;
                }

                if (i == (whatLength - 1))
                    return startInclusive;
            }

            if (len == whatLength) // sub string can't fit anymore.
                return nullptr;

            --len;
            ++startInclusive;
        }

        return nullptr;
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, wchar_t what)
    {
        return FindCaseInsensitive(startInclusive, startInclusive + strlen(startInclusive), what);
    }

    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what)
    {
        size_t len = endExclusive - startInclusive;

        if (len == 0)
            return nullptr;

        wchar_t upperWhat = safe_static_cast<wchar_t, uint32_t>(upperCaseSIMD(what));

        while (startInclusive < endExclusive) {
            if (upperCaseSIMD(*startInclusive) == upperWhat) {
                return startInclusive;
            }

            ++startInclusive;
        }

        return nullptr;
    }

    bool StringToBool(const wchar_t* str)
    {
        if (IsNumeric(str)) {
            int32_t val = StringToInt<int32_t>(str);
            // anything not zero is true.
            return val != 0;
        }

        if (core::strUtil::IsEqualCaseInsen(str, L"true")) {
            return true;
        }

        return false;
    }

    bool StringToBool(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        if (IsNumeric(startInclusive, endExclusive)) {
            int32_t val = StringToInt<int32_t>(startInclusive, endExclusive);
            // anything not zero is true.
            return val != 0;
        }

        if (core::strUtil::IsEqualCaseInsen(startInclusive, endExclusive, L"true")) {
            return true;
        }

        return false;
    }

    bool HasFileExtension(const wchar_t* path)
    {
        return FileExtension(path) != nullptr;
    }

    bool HasFileExtension(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        return FileExtension(startInclusive, endExclusive) != nullptr;
    }

    const wchar_t* FileExtension(const wchar_t* path)
    {
        return FileExtension(path, path + strUtil::strlen(path));
    }
    const wchar_t* FileExtension(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        const wchar_t* res = strUtil::FindLast(startInclusive, endExclusive, '.');
        // I think it might be better to return null here, instead of start.
        if (!res || res == (endExclusive - 1)) {
            return nullptr;
        }
        return res + 1;
    }

    const wchar_t* FileName(const wchar_t* path)
    {
        return FileName(path, path + strUtil::strlen(path));
    }

    const wchar_t* FileName(const wchar_t* startInclusive, const wchar_t* endExclusive)
    {
        // make sure slash is correct.
        X_ASSERT(Find(startInclusive, endExclusive, Path<wchar_t>::INVALID_SLASH) == nullptr, "Path Invalid slash")();

        const wchar_t* res = strUtil::FindLast(startInclusive, endExclusive, Path<wchar_t>::SLASH_W);

        if (!res || res == (endExclusive - 1)) {
            return startInclusive; // might just be file name.
        }
        return res + 1;
    }

    bool WildCompare(const wchar_t* wild, const wchar_t* string)
    {
        const wchar_t *cp = nullptr, *mp = nullptr;

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

    size_t LineNumberForOffset(const wchar_t* pBegin, const wchar_t* pEnd, size_t offset)
    {
        X_ASSERT(pBegin <= pEnd, "Invalid range")(pBegin, pEnd);

        size_t size = (pEnd - pBegin);
        if (offset > size) {
            X_WARNING("", "Offset out of range. size %" PRIuS " offset: %" PRIuS, size, offset);
            return 1;
        }

        size_t line = 1; // :| !
        for (size_t i = 0; i < offset; i++) {
            line += (pBegin[i] == L'\n');
        }

        return line;
    }

    size_t NumLines(const wchar_t* pBegin, const wchar_t* pEnd)
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