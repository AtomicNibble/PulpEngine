#pragma once


#ifndef X_STRING_JSON_H_
#define X_STRING_JSON_H_

#if X_COMPILER_CLANG
#ifdef _MSC_VER
#undef _MSC_VER
#endif
#endif

#define RAPIDJSON_NAMESPACE X_NAMESPACE_NAME::core::json
#define RAPIDJSON_NAMESPACE_BEGIN namespace X_NAMESPACE_NAME { namespace core {  namespace json {
#define RAPIDJSON_NAMESPACE_END   } } }

#define RAPIDJSON_ASSERT(x) X_ASSERT(x,"Json")(x);

#if X_64
	#define RAPIDJSON_SSE2
#endif // !X_^$

#include <../../3rdparty/source/rapidjson/rapidjson.h>
#include <../../3rdparty/source/rapidjson/document.h>
#include <../../3rdparty/source/rapidjson/writer.h>
#include <../../3rdparty/source/rapidjson/reader.h>
#include <../../3rdparty/source/rapidjson/stringbuffer.h>
#include <../../3rdparty/source/rapidjson/error/en.h>


RAPIDJSON_NAMESPACE_BEGIN

typedef core::StackString512 Description;

inline const char* ErrorToString(const Document& d, Description& desc)
{
	auto err = d.GetParseError();
	const char* pErrStr = GetParseError_En(err);
	size_t offset = d.GetErrorOffset();

	desc.appendFmt("desc(%" PRIi32 ") : Offset: %" PRIuS " Err : %s", err, offset, pErrStr);
	return desc.c_str();
}

inline const char* ErrorToString(const Document& d, const char* pBegin, const char* pEnd, Description& desc)
{
	auto err = d.GetParseError();
	const char* pErrStr = GetParseError_En(err);
	size_t offset = d.GetErrorOffset();
	size_t line = strUtil::LineNumberForOffset(pBegin, pEnd, offset);

	desc.appendFmt("desc(%" PRIi32 ") : Offset: %" PRIuS " Line : %" PRIuS " Err : %s", err, offset, line, pErrStr);
	return desc.c_str();
}

RAPIDJSON_NAMESPACE_END

#endif // !X_STRING_JSON_H_