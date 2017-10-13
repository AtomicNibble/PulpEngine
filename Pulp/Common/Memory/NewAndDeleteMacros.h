#pragma once

#ifndef X_NEWANDDELETEMACROS_H_
#define X_NEWANDDELETEMACROS_H_


/// \def X_NEW
/// \ingroup Memory
/// \brief Creates a new instance, similar to calling \c new.
/// \details The macro creates a new instance of a given type using placement new. It first allocates memory for the
/// instance, and then uses placement new to construct the instance. The macro also honors the required alignment
/// of the given type. The provided \a ID can be used to identify allocations.
///
/// The arguments to a constructor can be supplied after calling the \ref X_NEW macro, shown in the following example:
/// \code
///   // allocates an int, similar to calling "new int"
///   int* anInt = X_NEW(int, someArena, "An integer");
///
///   // allocates an instance of a custom data structure, supplying arguments to the constructor.
///   // this also behaves the same way as calling "new DataStruct(arg1, arg2, arg3)".
///   DataStruct* data = X_NEW(DataStruct, someArena, "A data structure")(arg1, arg2, arg3);
/// \endcode
/// The macro generates additional information which is useful for memory tracking and debugging purposes. Such information
/// includes a string describing the type, as well as an instance of \ref X_SOURCE_INFO which stores the file name and
/// line number where the allocation has been made. This information is used by certain memory arenas and debugging facilities.
/// \remark The given \a ID must be a constant string/string literal.
/// \sa X_NEW_ALIGNED X_NEW_ARRAY X_DELETE
#define X_NEW(type, arena, ID)											X_NEW_ALIGNED(type, arena, ID, X_ALIGN_OF(type))


/// \def X_NEW_ALIGNED
/// \ingroup Memory
/// \brief Creates a new aligned instance, similar to calling \c new.
/// \details The macro behaves the same way as \ref X_NEW, but takes an additional parameter that specifies the alignment
/// of the created instance.
/// \sa X_NEW X_NEW_ARRAY_ALIGNED X_DELETE
#define X_NEW_ALIGNED(type, arena, ID, alignment)		 				new (X_NAMESPACE(core)::Mem::New<type>(arena, alignment, ID, X_PP_STRINGIZE(type) X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::Mem::IsDerivedFromBaseArena<type>::Value>())) type
//new ((arena)->allocate(sizeof(type), alignment, 0, ID, X_PP_STRINGIZE(type), X_SOURCE_INFO)) type


/// \def X_NEW_ARRAY
/// \ingroup Memory
/// \brief Creates an array of instances, similar to calling \c new[].
/// \details The macro creates an array of instances of a given type using placement new. It first allocates memory for the
/// array, and then uses placement new to construct the instances. The macro also honors the required alignment
/// of the given type. The provided \a ID can be used to identify allocations.
///
/// The macro generates additional information which is useful for memory tracking and debugging purposes. Such information
/// includes a string describing the type, as well as an instance of \ref X_SOURCE_INFO which stores the file name and
/// line number where the allocation has been made. This information is used by certain memory arenas and debugging facilities.
/// \remark The given \a ID must be a constant string/string literal.
/// \sa X_NEW_ARRAY_ALIGNED X_NEW X_DELETE_ARRAY
#define X_NEW_ARRAY(type, count, arena, ID)							X_NEW_ARRAY_ALIGNED(type, count, arena, ID, X_ALIGN_OF(type))



/// \def X_NEW_ARRAY_ALIGNED
/// \ingroup Memory
/// \brief Creates an array of aligned instances, similar to calling \c new[].
/// \details The macro behaves the same way as \ref X_NEW_ARRAY, but takes an additional parameter that specifies the alignment
/// of the created instances.
/// \sa X_NEW_ARRAY X_NEW X_DELETE_ARRAY
#define X_NEW_ARRAY_ALIGNED(type, count, arena, ID, alignment)			X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, alignment, ID, X_PP_STRINGIZE(type) "[" X_PP_STRINGIZE(count) "]" X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())
#define X_NEW_ARRAY_OFFSET(type, count, arena, ID, offset)				X_NAMESPACE(core)::Mem::NewArray<type>(arena, count, X_ALIGN_OF(type), offset, ID, X_PP_STRINGIZE(type) "[" X_PP_STRINGIZE(count) "]" X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO), X_NAMESPACE(core)::compileTime::IntToType<X_NAMESPACE(core)::compileTime::IsPOD<type>::Value>())


/// \def X_DELETE
/// \ingroup Memory
/// \brief Deletes an instance, similar to calling \c delete.
/// \details The macro deletes a given instance by calling the destructor and freeing the memory.
/// \sa X_NEW X_NEW_ALIGNED X_DELETE_ARRAY
#define X_DELETE(object, arena)										(object != nullptr) ? X_NAMESPACE(core)::Mem::Delete(object, arena) : X_UNUSED(true)
#define X_DELETE_AND_NULL(object, arena)							(object != nullptr) ? X_NAMESPACE(core)::Mem::DeleteAndNull(object, arena) : X_UNUSED(true)


/// \def X_DELETE_ARRAY
/// \ingroup Memory
/// \brief Deletes an array of instances, similar to calling \c delete[].
/// \details The macro deletes an array of instances by calling their destructors and freeing the memory.
/// \sa X_NEW_ARRAY X_NEW_ARRAY_ALIGNED X_DELETE
#define X_DELETE_ARRAY(object, arena)									(object != nullptr) ? X_NAMESPACE(core)::Mem::DeleteArray(object, arena) : X_UNUSED(true)


#endif
