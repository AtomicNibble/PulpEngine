#pragma once

#include <String\StringUtil.h>

X_NAMESPACE_BEGIN(core)

template<typename TChar>
class StringRef;

template<size_t N, typename TChar>
class StackString;


class string_view
{
public:
    using value_type = char;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type &;
    using const_reference = const value_type&;
    using const_iterator = const_pointer;
    using iterator = const_iterator;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

public:
    constexpr string_view();
    constexpr string_view(const string_view&) = default;
    constexpr explicit string_view(const_pointer pStr);
    constexpr string_view(const_pointer pStr, size_type length);
    constexpr string_view(const_pointer pBegin, const_pointer pEnd);

    explicit string_view(const StringRef<value_type>& str);

    template<size_t N>
    constexpr string_view(const StackString<N, value_type>& str);

    constexpr string_view& operator=(const string_view&) = default;

    constexpr const_reference operator[](const size_type idx) const;
    constexpr const_reference at(const size_type idx) const;

    constexpr size_type size() const;
    constexpr size_type length() const;
    constexpr bool empty() const;
    constexpr const_pointer data() const;

    constexpr const_iterator begin() const;
    constexpr const_iterator end() const;

private:
    const_pointer pBegin_;
    size_type size_;
};


constexpr string_view::string_view() :
    pBegin_(nullptr),
    size_(0)
{
}

constexpr string_view::string_view(const_pointer pStr) :
    pBegin_(pStr),
    size_(__builtin_strlen(pStr)) // Visual studio seams to have this?
{
}

constexpr string_view::string_view(const_pointer pStr, size_type length) :
    pBegin_(pStr),
    size_(length)
{
}

constexpr string_view::string_view(const_pointer pBegin, const_pointer pEnd) :
    pBegin_(pBegin),
    size_(pBegin - pEnd)
{
}

template<size_t N>
constexpr string_view::string_view(const StackString<N, value_type>& str) :
    pBegin_(str.data()),
    size_(str.length())
{
}

constexpr typename string_view::const_reference string_view::operator[](const size_type idx) const
{
    return *(pBegin_ + idx);
}

constexpr typename string_view::const_reference string_view::at(const size_type idx) const
{
    return *(pBegin_ + idx);
}

constexpr string_view::size_type string_view::size() const
{
    return size_;
}

constexpr string_view::size_type string_view::length() const
{
    return size_;
}

constexpr bool string_view::empty() const
{
    return size_ == 0;
}

constexpr typename string_view::const_pointer string_view::data() const
{
    return pBegin_;
}

constexpr typename string_view::const_iterator string_view::begin() const
{
    return pBegin_;
}

constexpr typename string_view::const_iterator string_view::end() const
{
    return pBegin_ + size_;
}


namespace string_view_literals
{
    X_NO_DISCARD constexpr string_view operator"" _sv(const char* pStr, size_t length) noexcept
    {
        return string_view(pStr, length);
    }

} // namespace string_view_literals 

X_NAMESPACE_END