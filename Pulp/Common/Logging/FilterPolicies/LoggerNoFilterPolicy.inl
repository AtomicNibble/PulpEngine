



inline void LoggerNoFilterPolicy::Init(void)
{

}

inline void LoggerNoFilterPolicy::Exit(void)
{

}

inline bool LoggerNoFilterPolicy::Filter(const char* type, const SourceInfo& sourceInfo, 
	const char* channel, size_t verbosity, const char* format, va_list args)
{
	X_UNUSED(type);
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(verbosity);
	X_UNUSED(format);
	X_UNUSED(args);
	return true;
}