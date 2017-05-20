#pragma once

#ifndef X_ENUM_MACRO_UTIL_H_
#define X_ENUM_MACRO_UTIL_H_

#include <Prepro\PreproNumArgs.h>
#include <Prepro\PreproStringize.h>
#include <Prepro\PreproExpandArgs.h>
#include <Prepro\PreproPassArgs.h>

#define X_DECLARE_ENUM_IMPL_ENUM(value, n)					value = n,
#define X_DECLARE_ENUM_IMPL_TO_STRING(value, n)			case value: return X_PP_STRINGIZE(value);

#define X_DECLARE_ENUM_IMPL(...)														\
	{																					\
		static const unsigned int ENUM_COUNT = X_PP_NUM_ARGS(__VA_ARGS__);				\
																						\
		enum Enum																		\
		{																				\
			X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_ENUM, __VA_ARGS__)		\
		};																				\
																						\
		static const char* ToString(uint32_t value)										\
		{																				\
		switch (value)																	\
			{																			\
			X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_TO_STRING, __VA_ARGS__)	\
			default:																	\
			X_NO_SWITCH_DEFAULT;														\
			}																			\
		}																				\
	}

#define X_DECLARE_ENUM8_IMPL(...)														\
	{																					\
		static const unsigned int ENUM_COUNT = X_PP_NUM_ARGS(__VA_ARGS__);				\
		enum Enum : uint8_t																\
		{																				\
			X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_ENUM, __VA_ARGS__)		\
		};																				\
		static const char* ToString(uint32_t value)										\
		{																				\
		switch (value)																	\
			{																			\
			X_PP_EXPAND_ARGS(X_DECLARE_ENUM_IMPL_TO_STRING, __VA_ARGS__)	\
			default:																	\
			X_NO_SWITCH_DEFAULT;														\
			}																			\
		}																				\
	}



#define X_DECLARE_ENUM(name) struct name X_DECLARE_ENUM_IMPL
#define X_DECLARE_ENUM8(name) struct name X_DECLARE_ENUM8_IMPL



#endif // X_ENUM_MACRO_UTIL_H_