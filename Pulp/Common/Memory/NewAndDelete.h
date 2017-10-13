#pragma once

#ifndef X_NEWANDDELETE_H_
#define X_NEWANDDELETE_H_

#include "CompileTime/IntToType.h"
#include "CompileTime/IsPOD.h"
#include <new>

#include <Memory\MemoryArenaBase.h>

X_NAMESPACE_BEGIN(core)

// i need definition now for sizeof + shiz.
//class MemoryArenaBase;

namespace Mem
{
	/// A type that represents the outcome of a compile-time value for a non-POD type.
	typedef compileTime::IntToType<false> NonPODType;
	typedef compileTime::IntToType<false> NoneArenaType;

	/// A type that represents the outcome of a compile-time value for a POD type.
	typedef compileTime::IntToType<true> PODType;
	typedef compileTime::IntToType<true> ArenaType;


	template <typename T, typename = void>
	struct IsDerivedFromBaseArena {
		static const bool Value = false;	
	};

	template <typename T>
	struct IsDerivedFromBaseArena<T, typename std::enable_if<std::is_base_of<MemoryArenaBase, T>::value>::type> { 
		static const bool Value = true; 
	};


	template <typename T>
	inline T* New(MemoryArenaBase* arena, size_t alignment
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), NoneArenaType);

	template <typename T>
	inline T* New(MemoryArenaBase* arena, size_t alignment, const char* ID,
		const char* typeName X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), ArenaType);


	/// \brief "new"'s an array of POD instances, similar to calling <tt>new POD[N]</tt>.
	/// \details Only allocates the memory needed for the instances, and does not call any constructors.
	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), PODType);

	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment, size_t offset
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), PODType);

	/// \brief "new"'s an array of non-POD instances, similar to calling <tt>new NonPOD[N]</tt>.
	/// \details Allocates memory needed by the instances, and calls the instances' constructors in order. Because
	/// the memory system needs to know how many instances are stored in the array upon freeing it, an additional 4 bytes
	/// are requested in order to store the array's size.
	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo), NonPODType);


	/// "delete"'s an instance of any type, similar to calling <tt>delete instance</tt>.
	template <typename T>
	inline void Delete(T* object, MemoryArenaBase* arena);
	
	template <typename T>
	inline void DeleteAndNull(T*& object, MemoryArenaBase* arena);


	/// \brief "delete"'s an array of instances, similar to calling <tt>delete[] instances</tt>.
	/// \details The function delegates the actual deletion of the array to the proper function based on the type of \a T.
	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena);

	/// \brief "delete"'s an array of POD instances, similar to calling <tt>delete[] instances</tt>.
	/// \details Only frees the memory, and does not call any destructors.
	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena, PODType);

	/// \brief "delete"'s an array of non-POD instances, similar to calling <tt>delete[] instances</tt>.
	/// \details Calls the destructors in reverse order, and frees the memory.
	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena, NonPODType);


	/// Constructs an instance in memory at the given address.
	template <typename T>
	inline T* Construct(void* where);

	/// Constructs an instance in memory at the given address, copying from another instance.
	template <typename T>
	inline T* Construct(void* where, const T& what);

	/// Constructs an instance in memory at the given address, copying from another instance.
	template <typename T>
	inline T* Construct(void* where, T&& what);

	/// Construct an instance from args
	template <typename T, class... _Types>
	inline T* Construct(void* where, _Types&&... _Args);

	/// Constructs N instances in memory at the given address, and returns a pointer to the first instance.
	template <typename T>
	inline T* ConstructArray(void* where, size_t N);

	template <typename T>
	inline T* ConstructArray(void* where, size_t N, const T& what);

	/// Copy array into uninitaliazlied memory.
	template <typename T>
	inline T* CopyArrayUninitialized(void* where, const T* fromBegin, const T* fromEnd);

	template <typename T>
	inline T* MoveArrayUninitialized(void* where, T* fromBegin, T* fromEnd);

	template <typename T>
	inline T* MoveArrayDestructUninitialized(void* where, T* fromBegin, T* fromEnd);

	/// \brief Destructs an instance in memory.
	/// \details The function delegates the actual destruction of the instance to the proper function based on the type of \a T.
	template <typename T>
	inline void Destruct(T* instance);

	/// Destructs a POD instance.
	template <typename T>
	inline void Destruct(T* instance, PODType);

	/// Destructs a non-POD instance.
	template <typename T>
	inline void Destruct(T* instance, NonPODType);

	template <typename T>
	inline void DestructArray(T* pInstances, size_t N, PODType);

	template <typename T>
	inline void DestructArray(T* pInstances, size_t N, NonPODType);

	/// Destructs N instances in memory.
	template <typename T>
	inline void DestructArray(T* instances, size_t N);
}

#include "NewAndDelete.inl"

X_NAMESPACE_END

#endif
