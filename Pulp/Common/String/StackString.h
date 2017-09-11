#pragma once

#ifndef X_FIXEDSIZESTRING_H_
#define X_FIXEDSIZESTRING_H_

#include "String\FormatMacros.h"

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
template <size_t N, typename TChar = char>
class StackString
{
public:
	typedef TChar char_type;

	static const size_t BUF_SIZE = N;

public:
	/// Constructs an empty string.
	inline StackString(void);

	inline StackString(const StackString<N,TChar>& oth);

	/// Constructs a string by copying the given string.
	explicit StackString(const TChar* const str);
	explicit StackString(const wchar_t* const str);

	/// Constructs a string by copying the given range.
	explicit StackString(const StringRange<TChar>& range);

	explicit StackString(const bool b);
	explicit StackString(const char c);
	explicit StackString(const wchar_t c);
	explicit StackString(const int i);
	explicit StackString(const unsigned u);
	explicit StackString(const float f);
	explicit StackString(const unsigned __int64 f);
	explicit StackString(const __int64 f);

	/// Constructs a string by copying the given range.
	StackString(const TChar* const beginInclusive, const TChar* const endExclusive);

	/// Appends a character a certain amount of times.
	void append(TChar ch, size_t count);
	/// Appends a string.
	inline void append(const TChar* str);
	/// Appends part of another string.
	void append(const TChar* str, size_t count);
	/// Appends part of another string.
	inline void append(const TChar* str, const TChar* end);
	/// Appends a formatted string.
	void appendFmt(const TChar* format, ...);
	void appendFmt(const TChar* format, va_list args);

	void set(const TChar* str);
	void set(const TChar* const beginInclusive, const TChar* const endExclusive);

	void setFmt(const TChar* format, ...);
	void setFmt(const TChar* format, va_list args);

	/// \brief Replaces part of the string.
	/// \remark Returns whether the string was found and replaced.
	bool replace(const TChar* original, const TChar* replacement);
	bool replace(const TChar* start, const TChar* original, const TChar* replacement);
	/// \brief Replaces first occurenct of character
	/// \remark Returns whether the string was found and replaced.
	bool replace(const TChar original, const TChar replacement);

	/// Replaces all occurrences of a string, and returns the number of occurrences replaced.
	size_t replaceAll(const TChar* original, const TChar* replacement);
	/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
	size_t replaceAll(const TChar original, const TChar replacement);


	/// Trims all whitespace to the left and right of the string.
	void trimWhitespace(void);
	/// Trims all occurrences of the given character to the left and right of the string.
	void trim(TChar character);

	void trimLeft(TChar ch);
	void trimLeft(const TChar* pos);
	
	void trimRight(TChar ch);
	void trimRight(const TChar* pos);

	StackString<N, TChar>& trim(void);
	/// removes all white space chars in front of string.
	StackString<N, TChar>& trimLeft(void);
	// removes any leading white space chars.
	StackString<N, TChar>& trimRight(void);


	/// \brief strip char from end as many times as the char occurs
	/// Same as trimRight.
	inline void stripTrailing(const TChar c);

	/// removes the color codes ^1 ..
	inline void stripColorCodes(void);

	/// \brief Clears the string such that GetLength() yields 0.
	/// \remark No assumptions about the characters stored in the internal array should be made.
	inline void clear(void);

	/// Returns whether the string equals a given string.
	inline bool isEqual(const TChar* other) const;


	/// \brief Finds a character in the string, and returns a pointer to the last occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const TChar* findLast(TChar ch) const;
	/// \brief Finds a character in the string, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const TChar* find(TChar ch) const;
	/// \brief Finds a string inside the string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	inline const TChar* find(const TChar* string) const;

	inline const TChar* findCaseInsen(TChar ch) const;
	inline const TChar* findCaseInsen(const TChar* string) const;
	


	inline bool operator==(const StackString& oth) const;
	inline bool operator!=(const StackString& oth) const;
	inline StackString& operator=(const StackString& oth);

	inline TChar& operator[](size_t i);
	inline const TChar& operator[](size_t i) const;

	inline const TChar* c_str(void) const;

	inline size_t length(void) const;
	inline constexpr size_t capacity(void) const;

	inline bool isEmpty(void) const;
	inline bool isNotEmpty(void) const;

	void toLower(void);
	void toUpper(void);

	inline const TChar* begin(void) const;
	inline const TChar* end(void) const;

protected:
	TChar str_[N];
	size_t len_;
};

template <size_t N>
class StackString<N,wchar_t>
{
public:
	/// Constructs an empty string.
	inline StackString(void);

	/// Constructs a string by copying the given string.
	explicit StackString(const wchar_t* const str);
	explicit StackString(const char* const str);

	/// Constructs a string by copying the given range.
	explicit StackString(const StringRange<wchar_t>& range);

	explicit StackString(const bool b);
	explicit StackString(const char c);
	explicit StackString(const wchar_t c);
	explicit StackString(const int i);
	explicit StackString(const unsigned u);
	explicit StackString(const float f);
	explicit StackString(const unsigned __int64 f);
	explicit StackString(const __int64 f);

	/// Constructs a string by copying the given range.
	StackString(const wchar_t* const beginInclusive, const wchar_t* const endExclusive);

	/// Appends a character a certain amount of times.
	void append(wchar_t ch, size_t count);
	/// Appends a string.
	inline void append(const wchar_t* str);
	/// Appends part of another string.
	void append(const wchar_t* str, size_t count);
	/// Appends part of another string.
	inline void append(const wchar_t* str, const wchar_t* end);
	/// Appends a formatted string.
	void appendFmt(const wchar_t* format, ...);
	void appendFmt(const wchar_t* format, va_list args);

	void set(const wchar_t* str);
	void set(const wchar_t* const beginInclusive, const wchar_t* const endExclusive);

	void setFmt(const wchar_t* format, ...);
	void setFmt(const wchar_t* format, va_list args);

	/// \brief Replaces part of the string.
	/// \remark Returns whether the string was found and replaced.
	bool replace(const wchar_t* original, const wchar_t* replacement);
	bool replace(const wchar_t* start, const wchar_t* original, const wchar_t* replacement);
	/// \brief Replaces first occurenct of character
	/// \remark Returns whether the string was found and replaced.
	bool replace(const wchar_t original, const wchar_t replacement);

	/// Replaces all occurrences of a string, and returns the number of occurrences replaced.
	size_t replaceAll(const wchar_t* original, const wchar_t* replacement);
	/// Replaces all occurrences of a character, and returns the number of occurrences replaced.
	size_t replaceAll(const wchar_t original, const wchar_t replacement);


	/// Trims all whitespace to the left and right of the string.
	void trimWhitespace(void);
	/// Trims all occurrences of the given character to the left and right of the string.
	void trim(wchar_t character);

	void trimLeft(wchar_t ch);
	void trimLeft(const wchar_t* pos);

	void trimRight(wchar_t ch);
	void trimRight(const wchar_t* pos);

	StackString<N, wchar_t>& trim(void);
	/// removes all white space chars in front of string.
	StackString<N, wchar_t>& trimLeft(void);
	// removes any leading white space chars.
	StackString<N, wchar_t>& trimRight(void);


	/// \brief strip char from end as many times as the char occurs
	inline void stripTrailing(const wchar_t c);

	/// removes the color codes ^1 ..
	inline void stripColorCodes(void);

	/// \brief Clears the string such that GetLength() yields 0.
	/// \remark No assumptions about the characters stored in the internal array should be made.
	inline void clear(void);

	/// Returns whether the string equals a given string.
	inline bool isEqual(const wchar_t* other) const;


	/// \brief Finds a character in the string, and returns a pointer to the last occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const wchar_t* findLast(wchar_t ch) const;
	/// \brief Finds a character in the string, and returns a pointer to the first occurrence of the character.
	/// \remark Returns a \c nullptr if the character could not be found.
	inline const wchar_t* find(wchar_t ch) const;
	/// \brief Finds a string inside the string, and returns a pointer to it.
	/// \remark Returns a \c nullptr if the string could not be found.
	inline const wchar_t* find(const wchar_t* string) const;

	inline const wchar_t* findCaseInsen(wchar_t ch) const;
	inline const wchar_t* findCaseInsen(const wchar_t* string) const;



	inline bool operator==(const StackString& oth) const;
	inline bool operator!=(const StackString& oth) const;
	inline StackString& operator=(const StackString& oth);

	inline wchar_t& operator[](size_t i);
	inline const wchar_t& operator[](size_t i) const;

	inline const wchar_t* c_str(void) const;

	inline size_t length(void) const;
	inline constexpr size_t capacity(void) const;

	inline bool isEmpty(void) const;
	inline bool isNotEmpty(void) const;

	void toLower(void);
	void toUpper(void);

	inline const wchar_t* begin(void) const;
	inline const wchar_t* end(void) const;

protected:
	wchar_t str_[N];
	size_t len_;
};


#include "StackString.inl"
#include "StackStringW.inl"


typedef StackString<512, char> StackString512;
typedef StackString<512, wchar_t> StackStringW512;
typedef StackString<256, char> StackString256;
typedef StackString<256, wchar_t> StackStringW256;


X_NAMESPACE_END


#endif // X_FIXEDSIZESTRING_H_
