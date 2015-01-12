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


	const char* bytesToHumanString(size_t numBytes);


	const char* workingDir(void);

	inline uint32_t strlen(const char* str);

	/// Returns whether or not the given character is a whitespace.
	inline bool IsWhitespace(char character);
	inline bool IsWhitespaceW(wchar_t character);

	/// Returns whether or not the given character is a digit.
	inline bool IsDigit(char character);
	inline bool IsDigitW(wchar_t character);

	inline bool IsNumeric(const char* str);


	/// Converts a wide-character string into a single-byte character string, and returns the converted string.
	template <size_t N>
	inline const char* Convert(const wchar_t* input, char (&output)[N]);

	/// Converts a wide-character string into a single-byte character string, and returns the converted string.
	const char* Convert(const wchar_t* input, char* output, uint32_t outputLength);

	/// Converts a wide-character string into a single-byte character string, and returns the converted string.
	const wchar_t* Convert(const char* input, wchar_t* output, uint32_t outputLength);


	/// Returns the number of occurrences of a character in a string in the given range.
	unsigned int Count(const char* startInclusive, const char* endExclusive, char what);


	/// Returns whether two strings are equal.
	bool IsEqual(const char* str1, const char* str2);

	/// Returns whether two strings are equal, checks the length of the 1sr range.
	bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2 );

	/// Returns whether two strings in their respective ranges are equal.
	bool IsEqual(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2);

	/// Returns whether two strings are equal. case-insensitive
	bool IsEqualCaseInsen(const char* str1, const char* str2);
	/// Returns whether two strings are equal, checks the length of the 1sr range.
	bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2);
	/// Returns whether two strings in their respective ranges are equal.
	bool IsEqualCaseInsen(const char* startInclusiveS1, const char* endExclusiveS1, const char* startInclusiveS2, const char* endExclusiveS2);


	/// \brief Finds a character in a string, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* Find(const char* startInclusive, const char* endExclusive, char what);

	/// \brief Finds the first character in a string that is not a certain character, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindNon(const char* startInclusive, const char* endExclusive, char what);

	/// \brief Finds a character in a string, and returns a pointer to the last occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindLast(const char* startInclusive, const char* endExclusive, char what);

	/// \brief Finds the last character in a string that is not a certain character, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the character could not be found.
	const char* FindLastNon(const char* startInclusive, const char* endExclusive, char what);

	/// \brief Finds a string inside a string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	const char* Find(const char* startInclusive, const char* endExclusive, const char* what);

	/// \brief Finds a string inside a string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	const char* Find(const char* startInclusive, const char* endExclusive, const char* what, unsigned whatLength);

	/// \brief Finds a string inside a string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	const char* Find(const char* startInclusive, const char* endExclusive, const char* whatStart, const char* whatEnd);

	/// \brief Finds a string inside a string using a case-insensitive search, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what);

	/// \brief Finds a string inside a string using a case-insensitive search, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	const char* FindCaseInsensitive(const char* startInclusive, const char* endExclusive, const char* what, size_t whatLength);
	
	const char* FindCaseInsensitive(const char* startInclusive, const char* what);



	/// \brief Finds the first whitespace character in a string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	const char* FindWhitespace(const char* startInclusive, const char* endExclusive);

	/// \brief Finds the last whitespace character in a string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	const char* FindLastWhitespace(const char* startInclusive, const char* endExclusive);

	/// \brief Finds the first character in a string that is not a whitespace, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	const char* FindNonWhitespace(const char* startInclusive, const char* endExclusive);

	/// \brief Finds the last character in a string that is not a whitespace, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	const char* FindLastNonWhitespace(const char* startInclusive, const char* endExclusive);


	/// \brief Converts a string into any integer type.
	/// \remark Returns 0 if the string does not represent a valid integer value.
	template <typename T>
	inline T StringToInt(const char* str);

	/// \brief Converts a string into any floating-point type.
	/// \remark Returns 0 if the string does not represent a valid floating-point value.
	template <typename T>
	inline T StringToFloat(const char* str);

	bool HasFileExtension(const char* path);
	bool HasFileExtension(const char* startInclusive, const char* endExclusive);

	// returns nullptr if not found.
	const char* FileExtension(const char* path);
	const char* FileExtension(const char* startInclusive, const char* endExclusive);

	const char* FileName(const char* path);
	const char* FileName(const char* startInclusive, const char* endExclusive);
	
	// retrusn true if the wild card search patten matches the string.
	bool WildCompare(const char* wild, const char* string);
}

#include "StringUtil.inl"

X_NAMESPACE_END


#endif // X_STRINGUTIL_H_
