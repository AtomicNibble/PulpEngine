#pragma once

#ifndef X_STRINGUTIL_H_
#define X_STRINGUTIL_H_

#include <stdlib.h>

#include <emmintrin.h>

#include <Util\BitUtil.h>

X_NAMESPACE_BEGIN(core)

namespace strUtil
{
    namespace Asm
    {
        //  these are not finished.
        //	extern "C" size_t x_strlen(const char * str);
        //	extern "C" int x_strcmp(const char* str1, const char* str2);
    }

    size_t strlen(const char* str);
    size_t strlen(const wchar_t* str);

    // return length of string in bytes
    template<typename T>
    size_t StringBytes(const T& str);
    // return length of string in bytes + null term.
    template<typename T>
    size_t StringBytesIncNull(const T& str);

    /// Returns whether or not the given character is a whitespace.
    inline bool IsWhitespace(const char character);
    inline bool IsWhitespaceW(const wchar_t character);

    // reutrns if character is decimal digit or upper / lower case letter.
    inline bool IsAlphaNum(const char str);
    inline bool IsAlphaNum(const uint8_t str);

    // reutrns if character is upper / lower case letter.
    inline bool IsAlpha(const char str);
    inline bool IsAlpha(const uint8_t str);

    /// Returns whether or not the given character is a digit.
    template<typename CharT>
    inline bool IsDigit(const CharT character);
    inline bool IsDigitW(const wchar_t character);

    template<typename CharT>
    inline bool IsNumeric(const CharT* str);
    template<typename CharT>
    inline bool IsNumeric(const CharT* startInclusive, const CharT* endExclusive);

    bool IsLower(const char character);
    bool IsLowerW(const wchar_t character);

    bool IsLower(const char* startInclusive);
    bool IsLower(const char* startInclusive, const char* endExclusive);

    bool IsLower(const wchar_t* startInclusive);
    bool IsLower(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// Converts a wide-character string into a single-byte character string, and returns the converted string.
    template<size_t N>
    inline const char* Convert(const wchar_t* input, char (&output)[N]);

    template<size_t N>
    inline const wchar_t* Convert(const char* input, wchar_t (&output)[N]);

    /// Converts a wide-character string into a single-byte character string, and returns the converted string.
    const char* Convert(const wchar_t* input, char* output, size_t outputBytes);

    /// Converts a wide-character string into a single-byte character string, and returns the converted string.
    const wchar_t* Convert(const char* input, wchar_t* output, size_t outputBytes);

    /// Returns the number of occurrences of a character in a string in the given range.
    unsigned int Count(const char* startInclusive, const char* endExclusive, char what);
    unsigned int Count(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// Returns whether two strings are equal.
    bool IsEqual(const char* str1, const char* str2);
    bool IsEqual(const wchar_t* str1, const wchar_t* str2);

    /// Returns whether two strings are equal, checks the length of both srings are equal.
    bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2);
    bool IsEqual(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2);

    /// Returns whether two strings in their respective ranges are equal.
    bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2);
    bool IsEqual(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2, const wchar_t* endExclusiveS2);

    /// Returns whether two strings are equal. case-insensitive
    bool IsEqualCaseInsen(const char* str1, const char* str2);
    bool IsEqualCaseInsen(const wchar_t* str1, const wchar_t* str2);
    /// Returns whether two strings are equal, checks the length of the 1sr range.
    bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2);
    bool IsEqualCaseInsen(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2);
    /// Returns whether two strings in their respective ranges are equal.
    bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2);
    bool IsEqualCaseInsen(const wchar_t* startInclusiveS1, const wchar_t* endExclusiveS1, const wchar_t* startInclusiveS2, const wchar_t* endExclusiveS2);

    const char* Find(const char* startInclusive, char what);
    const wchar_t* Find(const wchar_t* startInclusive, wchar_t what);

    /// \brief Finds a character in a string, and returns a pointer to the first occurrence of the character.
    /// \remark Returns a \c nullptr if the character could not be found.
    const char* Find(const char* startInclusive, const char* endExclusive, char what);
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// \brief Finds a string inside a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* Find(const char* startInclusive, const char* what);
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* what);

    /// \brief Finds a string inside a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* Find(const char* startInclusive, const char* endExclusive, const char* what);
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what);

    /// \brief Finds a string inside a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* Find(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength);
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what, size_t whatLength);

    /// \brief Finds a string inside a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* Find(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd);
    const wchar_t* Find(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* whatStart, const wchar_t* whatEnd);

    /// \brief Finds the first character in a string that is not a certain character, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the character could not be found.
    const char* FindNon(const char* startInclusive, const char* endExclusive, char what);
    const wchar_t* FindNon(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// \brief Finds a character in a string, and returns a pointer to the last occurrence of the character.
    /// \remark Returns a \c nullptr if the character could not be found.
    const char* FindLast(const char* startInclusive, const char* endExclusive, char what);
    const wchar_t* FindLast(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// \brief Finds the last character in a string that is not a certain character, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the character could not be found.
    const char* FindLastNon(const char* startInclusive, const char* endExclusive, char what);
    const wchar_t* FindLastNon(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// \brief Finds a string inside a string using a case-insensitive search, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what);

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* whatStart, const wchar_t* whatEnd);

    /// \brief Finds a string inside a string using a case-insensitive search, and returns a pointer to it.
    /// \remark Returns a \c nullptr if the string could not be found.
    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, const wchar_t* what, size_t whatLength);

    const char* FindCaseInsensitive(const char* startInclusive, const char* what);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* what);

    const char* FindCaseInsensitive(const char* startInclusive, char what);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, wchar_t what);

    const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, char what);
    const wchar_t* FindCaseInsensitive(const wchar_t* startInclusive, const wchar_t* endExclusive, wchar_t what);

    /// \brief Finds the first whitespace character in a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if no such character could not be found.
    const char* FindWhitespace(const char* startInclusive, const char* endExclusive);
    const wchar_t* FindWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// \brief Finds the last whitespace character in a string, and returns a pointer to it.
    /// \remark Returns a \c nullptr if no such character could not be found.
    const char* FindLastWhitespace(const char* startInclusive, const char* endExclusive);
    const wchar_t* FindLastWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// \brief Finds the first character in a string that is not a whitespace, and returns a pointer to it.
    /// \remark Returns a \c nullptr if no such character could not be found.
    const char* FindNonWhitespace(const char* startInclusive, const char* endExclusive);
    const wchar_t* FindNonWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// \brief Finds the last character in a string that is not a whitespace, and returns a pointer to it.
    /// \remark Returns a \c nullptr if no such character could not be found.
    const char* FindLastNonWhitespace(const char* startInclusive, const char* endExclusive);
    const wchar_t* FindLastNonWhitespace(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// \brief Converts a string into any integer type.
    /// \remark Returns 0 if the string does not represent a valid integer value.
    template<typename T>
    inline T StringToInt(const char* str);
    template<typename T>
    inline T StringToInt(const wchar_t* str);

    template<typename T>
    inline T StringToInt(const char* str, int32_t base);
    template<typename T>
    inline T StringToInt(const wchar_t* str, int32_t base);

    template<typename T>
    inline T StringToInt(const char* str, const char** pEndPtr, int32_t base);
    template<typename T>
    inline T StringToInt(const wchar_t* str, const wchar_t** pEndPtr, int32_t base);

    template<typename T>
    inline T StringToInt(const char* startInclusive, const char* endExclusive);
    template<typename T>
    inline T StringToInt(const wchar_t* startInclusive, const wchar_t* endExclusive);

    /// \brief Converts a string into any floating-point type.
    /// \remark Returns 0 if the string does not represent a valid floating-point value.
    template<typename T>
    inline T StringToFloat(const char* str);
    template<typename T>
    inline T StringToFloat(const wchar_t* str);

    template<typename T>
    inline T StringToFloat(const char* str, const char** pEndPtr);
    template<typename T>
    inline T StringToFloat(const wchar_t* str, const wchar_t** pEndPtr);

    bool StringToBool(const char* str);
    bool StringToBool(const char* startInclusive, const char* endExclusive);

    bool StringToBool(const wchar_t* str);
    bool StringToBool(const wchar_t* startInclusive, const wchar_t* endExclusive);

    bool HasFileExtension(const char* path);
    bool HasFileExtension(const wchar_t* path);
    bool HasFileExtension(const char* startInclusive, const char* endExclusive);
    bool HasFileExtension(const wchar_t* startInclusive, const wchar_t* endExclusive);

    // returns nullptr if not found.
    const char* FileExtension(const char* path);
    const wchar_t* FileExtension(const wchar_t* path);
    const char* FileExtension(const char* startInclusive, const char* endExclusive);
    const wchar_t* FileExtension(const wchar_t* startInclusive, const wchar_t* endExclusive);

    const char* FileName(const char* path);
    const wchar_t* FileName(const wchar_t* path);
    const char* FileName(const char* startInclusive, const char* endExclusive);
    const wchar_t* FileName(const wchar_t* startInclusive, const wchar_t* endExclusive);

    // retrusn true if the wild card search patten matches the string.
    bool WildCompare(const char* wild, const char* string);
    bool WildCompare(const wchar_t* wild, const wchar_t* string);

    size_t LineNumberForOffset(const char* pBegin, const char* pEnd, size_t offset);
    size_t LineNumberForOffset(const wchar_t* pBegin, const wchar_t* pEnd, size_t offset);

    size_t NumLines(const char* pBegin, const char* pEnd);
    size_t NumLines(const wchar_t* pBegin, const wchar_t* pEnd);

} // namespace strUtil

#include "StringUtil.inl"

X_NAMESPACE_END

#endif // X_STRINGUTIL_H_
