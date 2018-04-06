
#pragma once
#ifndef X_STRINGTOKENIZER_H
#define X_STRINGTOKENIZER_H

#include "String/StringRange.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup String
/// \class StringTokenizer
/// \brief A simple string tokenizer.
/// \details This class is most often used in conjunction with string ranges, e.g. when parsing text-files.
/// No characters are copied, and no memory is ever allocated, making this class very lightweight. In addition, tokens
/// extracted by the string tokenizer never contain any whitespace characters, making it suitable for parsing many
/// different input formats without choking on input with slightly wrong syntax.
/// Check the provided code example for example usage:
/// \code
///   // assume lineStart and lineEnd point to the start and end of e.g. "variableName = 10", respectively
///   core::StringTokenizer tokenizer(lineStart, lineEnd, '=');
///   core::StringRange valueName(nullptr, nullptr);
///   const bool foundValueName = tokenizer.ExtractToken(valueName);
///   core::StringRange value(nullptr, nullptr);
///   const bool foundValue = tokenizer.ExtractToken(value);
///
///   if (foundValueName && foundValue)
///   {
///     // valueName contains "variableName"
///     // value contains "10"
///     ...
///   }
/// \endcode
/// \remark The range of characters held by the tokenizer is defined as [begin, end).
/// \sa FixedSizeString StringRange
template<typename TChar = char>
class StringTokenizer
{
public:
    /// \brief Constructs a string tokenizer for the given range of characters.
    /// \remark Ownership of the provided arguments stays at the calling site.
    StringTokenizer(const TChar* startInclusive, const TChar* endExclusive, TChar delimiter);

    /// \brief Tries to extract the next token, and returns whether a token could be found or not.
    /// \remark If no token could be extracted, no assumptions should be made about the contents of \a range.
    bool ExtractToken(StringRange<TChar>& range);

private:
    const TChar* start_;
    const TChar* end_;
    TChar delimiter_;
};

X_NAMESPACE_END

#endif
