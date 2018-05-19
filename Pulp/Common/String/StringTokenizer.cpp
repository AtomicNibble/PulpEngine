#include "EngineCommon.h"
#include "StringRange.h"
#include "StringTokenizer.h"

X_NAMESPACE_BEGIN(core)

template<typename TChar>
StringTokenizer<TChar>::StringTokenizer(const TChar* startInclusive, const TChar* endExclusive,
    TChar delimiter) :
    start_(startInclusive),
    end_(endExclusive),
    delimiter_(delimiter)
{
    while (*start_ == delimiter && start_ < end_) {
        ++start_;
    }
}

template<typename TChar>
bool StringTokenizer<TChar>::extractToken(StringRange<TChar>& range)
{
    bool result = false;

    const TChar* lastNonWhitespace;
    const TChar* nonWhitespace;
    const TChar* tokenEnd;
    const TChar* tokenBegin;
    const TChar* lastnon;

    if (start_ < end_) {
        tokenBegin = start_;

        while (*start_ != delimiter_ && start_ < end_) {
            ++start_;
        }

        tokenEnd = start_;

        while (*start_ == delimiter_ && start_ < end_) {
            ++start_;
        }

        nonWhitespace = strUtil::FindNonWhitespace(tokenBegin, tokenEnd);

        if (nonWhitespace) {
            tokenBegin = nonWhitespace;
        }

        lastnon = strUtil::FindLastNonWhitespace(tokenBegin, tokenEnd);

        lastNonWhitespace = lastnon + 1;

        if (lastnon != nullptr) {
            tokenEnd = lastNonWhitespace;
        }

        range = StringRange<TChar>(tokenBegin, tokenEnd);

        result = true;
    }

    return result;
}

template class StringTokenizer<char>;
template class StringTokenizer<wchar_t>;

X_NAMESPACE_END