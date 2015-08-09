

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline StringRange<TChar>::StringRange(const TChar* const startInclusive, 
	const TChar* const endExclusive) :
	m_start(startInclusive), m_end(endExclusive)
{
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline StringRange<TChar>::StringRange(const TChar* const startInclusive, size_t length) :
	m_start(startInclusive), m_end(startInclusive + length)
{

}
// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::Find(TChar character) const
{
	return strUtil::Find(m_start, m_end, character);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::Find(const TChar* sub) const
{
	return strUtil::Find(m_start, m_end, sub);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::FindWhitespace(void) const
{
	return strUtil::FindWhitespace(m_start, m_end);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::FindNonWhitespace(void) const
{
	return strUtil::FindNonWhitespace(m_start, m_end);
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline TChar StringRange<TChar>::operator[](size_t i) const
{
	X_ASSERT(m_start + i < m_end, "Character %d cannot be accessed. Subscript out of range.", i)(GetLength(), m_start, static_cast<const void*>(m_end));
	return m_start[i];
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::GetStart(void) const
{
	return m_start;
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline const TChar* StringRange<TChar>::GetEnd(void) const
{
	return m_end;
}

// ---------------------------------------------------------------------------------------------------------------------
template<typename TChar>
inline size_t StringRange<TChar>::GetLength(void) const
{
	return safe_static_cast<size_t>(m_end - m_start);
}