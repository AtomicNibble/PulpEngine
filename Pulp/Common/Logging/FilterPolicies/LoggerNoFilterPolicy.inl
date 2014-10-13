



inline void LoggerNoFilterPolicy::Init(void)
{

}

inline void LoggerNoFilterPolicy::Exit(void)
{

}

inline bool LoggerNoFilterPolicy::Filter(const char* type, const SourceInfo& sourceInfo, const char* channel, size_t verbosity, const char* format, va_list args)
{
	return true;
}