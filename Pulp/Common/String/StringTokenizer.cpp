#include "EngineCommon.h"
#include "StringRange.h"
#include "StringTokenizer.h"

X_NAMESPACE_BEGIN(core)

/// \brief Constructs a string tokenizer for the given range of characters.
/// \remark Ownership of the provided arguments stays at the calling site.
template<typename TChar>
StringTokenizer<TChar>::StringTokenizer(const TChar* startInclusive, const TChar* endExclusive,
    TChar delimiter) :
    start_(startInclusive),
    end_(endExclusive),
    delimiter_(delimiter)
{
    while (*start_ == delimiter && start_ < end_)
        ++start_;
}

/// \brief Tries to extract the next token, and returns whether a token could be found or not.
/// \remark If no token could be extracted, no assumptions should be made about the contents of \a range.
template<typename TChar>
bool StringTokenizer<TChar>::ExtractToken(StringRange<TChar>& range)
{
    bool result = false;

    const TChar* lastNonWhitespace;
    const TChar* nonWhitespace;
    const TChar* tokenEnd;
    const TChar* tokenBegin;
    const TChar* lastnon;

    if (start_ < end_) {
        tokenBegin = this->start_;

        while (*this->start_ != this->delimiter_ && this->start_ < this->end_)
            ++this->start_;

        tokenEnd = this->start_;

        while (*this->start_ == this->delimiter_ && this->start_ < this->end_)
            ++this->start_;

        nonWhitespace = strUtil::FindNonWhitespace(tokenBegin, tokenEnd);

        if (nonWhitespace)
            tokenBegin = nonWhitespace;

        lastnon = strUtil::FindLastNonWhitespace(tokenBegin, tokenEnd);

        lastNonWhitespace = lastnon + 1;

        if (lastnon != nullptr)
            tokenEnd = lastNonWhitespace;

        range = StringRange<TChar>(tokenBegin, tokenEnd);

        result = true;
    }

    return result;
}

template class StringTokenizer<char>;
template class StringTokenizer<wchar_t>;

X_NAMESPACE_END