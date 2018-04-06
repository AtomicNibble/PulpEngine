#pragma once

#ifndef X_NEWANDDELETEMACROS_H_
#define X_NEWANDDELETEMACROS_H_

#define X_NEW(type, arena, ID) X_NEW_ALIGNED(type, arena, ID, X_ALIGN_OF(type))
#define X_NEW_ARRAY(type, count, arena, ID) X_NEW_ARRAY_ALIGNED(type, count, arena, ID, X_ALIGN_OF(type))

#if X_ENABLE_MEMORY_HUMAN_IDS

#define X_MEM_IDS(ID, type) , ID, type

#define X_NEW_ALIGNED(type, arena, ID, alignment) new (X_NAMESPACE(core)::Mem::New<type>(arena, alignment, ID, X_PP_STRINGIZE(type) X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::Mem::IsDerivedFromBaseArena<type>::Value>())) type

#define X_NEW_ARRAY_ALIGNED(type, count, arena, ID, alignment) X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, alignment, ID, X_PP_STRINGIZE(type) "[" X_PP_STRINGIZE(count) "]" X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())
#define X_NEW_ARRAY_OFFSET(type, count, arena, ID, offset) X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, X_ALIGN_OF(type), offset, ID, X_PP_STRINGIZE(type) "[" X_PP_STRINGIZE(count) "]" X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())

#else

#define X_MEM_IDS(ID, type)

#define X_NEW_ALIGNED(type, arena, ID, alignment) new (X_NAMESPACE(core)::Mem::New<type>(arena, alignment X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::Mem::IsDerivedFromBaseArena<type>::Value>())) type

#define X_NEW_ARRAY_ALIGNED(type, count, arena, ID, alignment) X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, alignment X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())
#define X_NEW_ARRAY_OFFSET(type, count, arena, ID, offset) X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, X_ALIGN_OF(type), offset X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())

#endif // !X_ENABLE_MEMORY_SOURCE_INFO

#define X_DELETE(object, arena) (object != nullptr) ? X_NAMESPACE(core)::Mem::Delete(object, arena) : X_UNUSED(true)
#define X_DELETE_AND_NULL(object, arena) (object != nullptr) ? X_NAMESPACE(core)::Mem::DeleteAndNull(object, arena) : X_UNUSED(true)
#define X_DELETE_ARRAY(object, arena) (object != nullptr) ? X_NAMESPACE(core)::Mem::DeleteArray(object, arena) : X_UNUSED(true)

#endif
