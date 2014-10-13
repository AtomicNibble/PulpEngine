

inline void LoggerBase::SetPrevious(LoggerBase* logger)
{
	m_previous = logger;
}

inline void LoggerBase::SetNext(LoggerBase* logger)
{
	m_next = logger;
}


inline LoggerBase* LoggerBase::GetPrevious(void) const
{
	return m_previous;
}


inline LoggerBase* LoggerBase::GetNext(void) const
{
	return m_next;
}

/// ----------------------------------------------------------------------------------

