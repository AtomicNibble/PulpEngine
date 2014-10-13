#pragma once

#ifndef X_FIXEDSIZESTRING_H_
#define X_FIXEDSIZESTRING_H_

#include "String/StringUtil.h"
#include "String/StringRange.h"

// #include <stdio.h>

X_NAMESPACE_BEGIN(core)

/// \ingroup String
/// \class FixedSizeString
/// \brief A class representing a string containing a fixed number of characters.
/// \details This class offers functionality similar to other string implementations, such as std::string.
/// However, as the name implies, a FixedSizeString will never grow, shrink, or allocate any memory dynamically. It can
/// often be used in situations where the maximum length of a string can be limited, such as when dealing with filenames,
/// in the logging system, when parsing files, etc. A FixedSizeString should be preferred to other implementations
/// in such cases.
///
/// In order to keep template instantiations to a minimum, certain typedefs for common usage scenarios are provided,
/// which should be used whenever possible.
/// \sa stringUtil StringRange FixedSizeString512 Pathname
template <size_t N>
class StackString
{
public:
	/// Constructs an empty string.
	inline StackString(void);

	/// Constructs a string by copying the given string.
	explicit StackString(const char* const str);

	/// Constructs a string by copying the given range.
	explicit StackString(const StringRange& range);

	explicit StackString(const bool b);
	explicit StackString(const char c);
	explicit StackString(const int i);
	explicit StackString(const unsigned u);
	explicit StackString(const float f);
	explicit StackString(const unsigned __int64 f);
	explicit StackString(const __int64 f);

	/// Constructs a string by copying the given range.
	StackString(const char* const beginInclusive, const char* const endExclusive);

	/// Appends a character a certain amount of times.
	void append(char ch, unsigned int count);
	/// Appends a string.
	inline void append(const char* str);
	/// Appends part of another string.
	void append(const char* str, unsigned int count);
	/// Appends part of another string.
	inline void append(const char* str, const char* end);
	/// Appends a formatted string.
	void appendFmt(const char* format, ...);
	void appendFmt(const char* format, va_list args);

	void set(const char* str);
	void set(const char* const beginInclusive, const char* const endExclusive);

	/// \brief Replaces part of the string.
	/// \remark Returns whether the string was found and replaced.
	bool replace(const char* original, const char* replacement);
	bool replace(const char* start, const char* original, const char* replacement);
	/// \brief Replaces first occurenct of character
	/// \remark Returns whether the string was found and replaced.
	bool replace(const char original, const char replacement);

	/// Replaces all occurrences of a string, and returns the number of occurrences replaced.
	unsigned int replaceAll(const char* original, const char* replacement);	
	/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
	unsigned int replaceAll(const char original, const char replacement);


	/// Trims all whitespace to the left and right of the string.
	void trimWhitespace(void);
	/// Trims all occurrences of the given character to the left and right of the string.
	void trimCharacter(char character);

	/// \brief Trims all characters to the right of the first occurrence of \a ch. The resulting string is [..., \a ch).
	/// \remark Does nothing if the character could not be found.
	void trimRight(char ch);
	/// \brief Trims all characters to the right of \a pos. The resulting string is [..., \a pos).
	inline void trimRight(const char* pos);

	StackString<N>& trim(void);
	/// removes all white space chars in front of string.
	StackString<N>& trimLeft(void);
	// removes any leading white space chars.
	StackString<N>& trimRight(void);


	/// \brief strip char from end as many times as the char occurs
	inline void stripTrailing( const char c );	


	/// \brief Clears the string such that GetLength() yields 0.
	/// \remark No assumptions about the characters stored in the internal array should be made.
	inline void clear(void);

	/// Returns whether the string equals a given string.
	inline bool isEqual(const char* other) const;


	/// \brief Finds a character in the string, and returns a pointer to the last occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const char* findLast(char ch) const;
	/// \brief Finds a character in the string, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const char* find(char ch) const;
	/// \brief Finds a string inside the string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	inline const char* find(const char* string) const;

	inline const char* findCaseInsen(char ch) const;
	inline const char* findCaseInsen(const char* string) const;
	


	inline bool operator==(const StackString& oth) const;
	inline bool operator!=(const StackString& oth) const;
	inline StackString& operator=(const StackString& oth);

	inline char& operator[](uint32_t i);
	inline const char& operator[](uint32_t i) const;

	inline const char* c_str(void) const;

	inline uint32_t length(void) const;
	inline uint32_t capacity(void) const;

	inline bool isEmpty(void) const;

	void toLower(void);
	void toUpper(void);

	inline const char* begin(void) const;
	inline const char* end(void) const;

protected:
	char str_[N];
	uint32_t len_;
};

#include "StackString.inl"


typedef StackString<512> StackString512;

X_NAMESPACE_END


#endif // X_FIXEDSIZESTRING_H_
