#pragma once
#ifndef X_STRINGRANGE_H
#define X_STRINGRANGE_H

#include "String/StringUtil.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup String
/// \class StringRange
/// \brief Represents a range of non-mutable characters.
/// \details This class is useful when dealing with strings inside an already allocated block of memory, such as when
/// parsing text-files or tokenizing strings because no characters are copied, and no memory is ever allocated.
/// \remark The range of characters held by the range is defined as [begin, end).
/// \sa FixedSizeString StringTokenizer
class StringRange
{
public:
	/// \brief Constructs a string range for the given range of characters.
	/// \remark Ownership of the provided arguments stays at the calling site.
	inline StringRange(const char* const startInclusive, const char* const endExclusive);

	/// \brief Constructs a string range for the given range of characters.
	/// \remark Ownership of the provided arguments stays at the calling site.
	inline StringRange(const char* const startInclusive, size_t length);

	/// \brief Finds a character in the range, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const char* Find(char character) const;

	/// \brief Finds a substring in the range, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const char* Find(const char* sub) const;

	/// \brief Finds the first whitespace character in the range, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	inline const char* FindWhitespace(void) const;

	/// \brief Finds the first character in the range that is not a whitespace, and returns a pointer to it.
	/// \remark Returns a \c nullptr if no such character could not be found.
	inline const char* FindNonWhitespace(void) const;

	/// \brief Returns the i-th non-mutable character.
	/// \remark The character is returned by-value rather than by-reference - this is intentional. A string
	/// range is a range of non-mutable characters because it never owns the characters it points to.
	inline char operator[](size_t i) const;

	/// Returns the start of the range.
	inline const char* GetStart(void) const;

	/// Returns the end of the range.
	inline const char* GetEnd(void) const;

	/// Returns the length of the range, not including the terminating null (if any).
	inline size_t GetLength(void) const;

private:
	const char* m_start;
	const char* m_end;
};

#include "StringRange.inl"

X_NAMESPACE_END


#endif
