#pragma once

// all logging should be done via these macro's
#if X_ENABLE_LOGGING

#	define X_LOG0(channel, format, ...)							( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Log(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, 0, format, ## __VA_ARGS__))
#	define X_LOG1(channel, format, ...)							( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Log(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, 1, format, ## __VA_ARGS__))
#	define X_LOG2(channel, format, ...)							( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Log(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, 2, format, ## __VA_ARGS__))
#	define X_WARNING(channel, format, ...)						( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Warning(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, format, ## __VA_ARGS__))
#	define X_ERROR(channel, format, ...)						( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Error(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, format, ## __VA_ARGS__))
#	define X_FATAL(channel, format, ...)						( gEnv == nullptr ? X_UNUSED(true) : gEnv->pLog->Fatal(X_SOURCE_INFO_LOG_CA(X_SOURCE_INFO) channel, format, ## __VA_ARGS__))
#	define X_LOG0_IF(condition, channel, format, ...)			((condition) ? X_LOG0(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_LOG1_IF(condition, channel, format, ...)			((condition) ? X_LOG1(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_LOG2_IF(condition, channel, format, ...)			((condition) ? X_LOG2(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_WARNING_IF(condition, channel, format, ...)		((condition) ? X_WARNING(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_ERROR_IF(condition, channel, format, ...)			((condition) ? X_ERROR(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_FATAL_IF(condition, channel, format, ...)			((condition) ? X_FATAL(channel, format, ## __VA_ARGS__) : X_UNUSED(true))
#	define X_LOG0_EVERY_N(N, channel, format, ...)																		\
	X_MULTILINE_MACRO_BEGIN																							\
	static unsigned int autoLogCount = 0;																			\
	X_LOG0_IF X_PP_PASS_ARGS(((autoLogCount++ % N) == 0), channel, format, __VA_ARGS__);							\
	X_MULTILINE_MACRO_END

#	define X_LOG1_EVERY_N(N, channel, format, ...)																		\
	X_MULTILINE_MACRO_BEGIN																							\
	static unsigned int autoLogCount = 0;																			\
	X_LOG1_IF X_PP_PASS_ARGS(((autoLogCount++ % N) == 0), channel, format, __VA_ARGS__);							\
	X_MULTILINE_MACRO_END

#	define X_LOG2_EVERY_N(N, channel, format, ...)																		\
	X_MULTILINE_MACRO_BEGIN																							\
	static unsigned int autoLogCount = 0;																			\
	X_LOG2_IF X_PP_PASS_ARGS(((autoLogCount++ % N) == 0), channel, format, __VA_ARGS__);							\
	X_MULTILINE_MACRO_END

#	define X_WARNING_EVERY_N(N, channel, format, ...)																		\
	X_MULTILINE_MACRO_BEGIN																							\
	static unsigned int autoLogCount = 0;																			\
	X_WARNING_IF X_PP_PASS_ARGS(((autoLogCount++ % N) == 0), channel, format, __VA_ARGS__);							\
	X_MULTILINE_MACRO_END


#	define X_LOG_BULLET				core::ILog::Bullet X_PP_UNIQUE_NAME(bullet)(gEnv ? gEnv->pLog : nullptr);


#else

#	define X_LOG0(channel, format, ...)										X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG1(channel, format, ...)										X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG2(channel, format, ...)										X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_WARNING(channel, format, ...)									X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_ERROR(channel, format, ...)									X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_FATAL(channel, format, ...)									X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG0_IF(condition, channel, format, ...)						X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG1_IF(condition, channel, format, ...)						X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG2_IF(condition, channel, format, ...)						X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_WARNING_IF(condition, channel, format, ...)					X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_ERROR_IF(condition, channel, format, ...)						X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_FATAL_IF(condition, channel, format, ...)						X_UNUSED(condition), X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG0_EVERY_N(N, channel, format, ...)							X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG1_EVERY_N(N, channel, format, ...)							X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG2_EVERY_N(N, channel, format, ...)							X_UNUSED(channel), X_UNUSED(format), X_UNUSED(__VA_ARGS__)
#	define X_LOG_BULLET

#endif
