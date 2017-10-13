



inline void LoggerNoFilterPolicy::Init(void)
{

}

inline void LoggerNoFilterPolicy::Exit(void)
{

}

inline void LoggerNoFilterPolicy::RegisterVars(void)
{

}

inline bool LoggerNoFilterPolicy::Filter(const char* type, X_SOURCE_INFO_LOG_CA(const SourceInfo&)
	const char* channel, int verbosity, const char* format, va_list args)
{
	X_UNUSED(type, channel, verbosity, format, args);
	return true;
}