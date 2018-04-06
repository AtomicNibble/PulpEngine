

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline StringRange<TChar>::StringRange(const TChar* const startInclusive,
    const TChar* const endExclusive) :
    start_(startInclusive),
    end_(endExclusive)
{
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline StringRange<TChar>::StringRange(const TChar* const startInclusive, size_t length) :
    start_(startInclusive),
    end_(startInclusive + length)
{
}
// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::Find(TChar character) const
{
    return strUtil::Find(start_, end_, character);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::Find(const TChar* sub) const
{
    return strUtil::Find(start_, end_, sub);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::FindWhitespace(void) const
{
    return strUtil::FindWhitespace(start_, end_);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::FindNonWhitespace(void) const
{
    return strUtil::FindNonWhitespace(start_, end_);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline TChar StringRange<TChar>::operator[](size_t i) const
{
    X_ASSERT(start_ + i < end_, "Character %d cannot be accessed. Subscript out of range.", i)(GetLength(), start_, static_cast<const void*>(end_));
    return start_[i];
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::GetStart(void) const
{
    return start_;
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::GetEnd(void) const
{
    return end_;
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline size_t StringRange<TChar>::GetLength(void) const
{
    return safe_static_cast<size_t>(end_ - start_);
}