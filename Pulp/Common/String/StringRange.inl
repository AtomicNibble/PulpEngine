

// ---------------------------------------------------------------------------------------------------------------------

inline StringRange::StringRange(const char* const startInclusive, const char* const endExclusive) :
	m_start(startInclusive), m_end(endExclusive)
{
}

// ---------------------------------------------------------------------------------------------------------------------
inline StringRange::StringRange(const char* const startInclusive, size_t length) :
	m_start(startInclusive), m_end(startInclusive + length)
{

}
// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::Find(char character) const
{
	return strUtil::Find(m_start, m_end, character);
}

// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::Find(const char* sub) const
{
	return strUtil::Find(m_start, m_end, sub);
}

// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::FindWhitespace(void) const
{
	return strUtil::FindWhitespace(m_start, m_end);
}

// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::FindNonWhitespace(void) const
{
	return strUtil::FindNonWhitespace(m_start, m_end);
}

// ---------------------------------------------------------------------------------------------------------------------
inline char StringRange::operator[](size_t i) const
{
	X_ASSERT(m_start + i < m_end, "Character %d cannot be accessed. Subscript out of range.", i)(GetLength(), m_start, static_cast<const void*>(m_end));
	return m_start[i];
}

// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::GetStart(void) const
{
	return m_start;
}

// ---------------------------------------------------------------------------------------------------------------------
inline const char* StringRange::GetEnd(void) const
{
	return m_end;
}

// ---------------------------------------------------------------------------------------------------------------------
inline size_t StringRange::GetLength(void) const
{
	return safe_static_cast<size_t>(m_end - m_start);
}