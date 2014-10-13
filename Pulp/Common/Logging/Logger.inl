
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
Logger<FilterPolicy, FormatPolicy, WritePolicy>::Logger(void) :
	LoggerBase(),
	m_filter(),
	m_formatter(),
	m_writer()
{
	m_filter.Init();
	m_formatter.Init();
	m_writer.Init();
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
Logger<FilterPolicy, FormatPolicy, WritePolicy>::Logger(const FilterPolicy& filter, const FormatPolicy& formatter, const WritePolicy& writer) :
	LoggerBase(),
	m_filter(filter),
	m_formatter(formatter),
	m_writer(writer)
{
	m_filter.Init();
	m_formatter.Init();
	m_writer.Init();
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
Logger<FilterPolicy, FormatPolicy, WritePolicy>::~Logger(void)
{
	m_filter.Exit();
	m_formatter.Exit();
	m_writer.Exit();
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoLog(const SourceInfo& sourceInfo, const char* channel, size_t verbosity, const char* format, va_list args)
{
	if (m_filter.Filter("INFO", sourceInfo, channel, verbosity, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "INFO", sourceInfo, channel, verbosity, format, args);
		m_writer.WriteLog(line, length);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoWarning(const SourceInfo& sourceInfo, const char* channel, const char* format, va_list args)
{
	if (m_filter.Filter("WARNING", sourceInfo, channel, 0, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "WARNING", sourceInfo, channel, 0, format, args);
		m_writer.WriteWarning(line, length);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoError(const SourceInfo& sourceInfo, const char* channel, const char* format, va_list args)
{
	if (m_filter.Filter("ERROR", sourceInfo, channel, 0, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "ERROR", sourceInfo, channel, 0, format, args);
		m_writer.WriteError(line, length);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoFatal(const SourceInfo& sourceInfo, const char* channel, const char* format, va_list args)
{
	if (m_filter.Filter("FATAL", sourceInfo, channel, 0, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "FATAL", sourceInfo, channel, 0, format, args);
		m_writer.WriteFatal(line, length);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoAssert(const SourceInfo& sourceInfo, const char* format, va_list args)
{
	if (m_filter.Filter("ASSERT", sourceInfo, "Assert", 0, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "ASSERT", sourceInfo, "Assert", 0, format, args);
		m_writer.WriteAssert(line, length);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
void Logger<FilterPolicy, FormatPolicy, WritePolicy>::DoAssertVariable(const SourceInfo& sourceInfo, const char* format, va_list args)
{
	if (m_filter.Filter("ASSERT", sourceInfo, "Assert", 0, format, args))
	{
		LoggerBase::Line line = {};
		const uint32_t length = m_formatter.Format(line, pLog_->GetIndentation(), "ASSERT", sourceInfo, "Assert", 0, format, args);
		m_writer.WriteAssertVariable(line, length);
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
const FilterPolicy& Logger<FilterPolicy, FormatPolicy, WritePolicy>::GetFilterPolicy(void) const
{
	return m_filter;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
const FormatPolicy& Logger<FilterPolicy, FormatPolicy, WritePolicy>::GetFormatPolicy(void) const
{
	return m_formatter;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class FilterPolicy, class FormatPolicy, class WritePolicy>
const WritePolicy& Logger<FilterPolicy, FormatPolicy, WritePolicy>::GetWritePolicy(void) const
{
	return m_writer;
}
