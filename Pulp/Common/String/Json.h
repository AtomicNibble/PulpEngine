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


#endif // !X_STRING_JSON_H_