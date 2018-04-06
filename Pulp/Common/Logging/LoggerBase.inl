

inline void LoggerBase::SetPrevious(LoggerBase* logger)
{
    previous_ = logger;
}

inline void LoggerBase::SetNext(LoggerBase* logger)
{
    next_ = logger;
}

inline LoggerBase* LoggerBase::GetPrevious(void) const
{
    return previous_;
}

inline LoggerBase* LoggerBase::GetNext(void) const
{
    return next_;
}

/// ----------------------------------------------------------------------------------
