
namespace Mem
{



	template <typename T>
	inline T* New(MemoryArenaBase* arena, size_t alignment, const char* ID,
		const char* typeName, const SourceInfo& sourceInfo, NoneArenaType)
	{
		X_ASSERT_NOT_NULL(arena);
		return static_cast<T*>(arena->allocate(sizeof(T), alignment, 0, ID, typeName, sourceInfo));
	}

	template <typename T>
	inline T* New(MemoryArenaBase* arena, size_t alignment, const char* ID,
		const char* typeName, const SourceInfo& sourceInfo, ArenaType)
	{
		X_ASSERT_NOT_NULL(arena);
		
		MemoryArenaBase* pNewArena = static_cast<MemoryArenaBase*>(arena->allocate(sizeof(T), alignment, 0, ID, typeName, sourceInfo));
		// we want a tree of allocators 
		// so we can calculate total usage for sub systems.
		// add it as a child node to the arena.
		arena->addChildArena(pNewArena);

		return static_cast<T*>(pNewArena);
	}


	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment, const char* ID, const char* typeName, const SourceInfo& sourceInfo, PODType)
	{
		X_ASSERT_NOT_NULL(arena);

		// no constructors need to be called for POD types
		return static_cast<T*>(arena->allocate(sizeof(T)*N, alignment, 0, ID, typeName, sourceInfo));
	}

	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment, size_t offset,
		const char* ID, const char* typeName, const SourceInfo& sourceInfo, PODType)
	{
		X_ASSERT_NOT_NULL(arena);

		// no constructors need to be called for POD types
		return static_cast<T*>(arena->allocate(sizeof(T)*N, alignment, offset, ID, typeName, sourceInfo));
	}





	template <typename T>
	inline T* NewArray(MemoryArenaBase* arena, size_t N, size_t alignment, const char* ID, const char* typeName, const SourceInfo& sourceInfo, NonPODType)
	{
		X_ASSERT_NOT_NULL(arena);

		union
		{
			void* as_void;
			uint32_t* as_uint32_t;
		};

	//	size_t size = sizeof(uint32_t);

		as_void = arena->allocate(sizeof(T)*N + sizeof(uint32_t), alignment, sizeof(uint32_t), ID, typeName, sourceInfo);

		// store the number of instances in the first 4 bytes
		*as_uint32_t++ = safe_static_cast<uint32_t,size_t>(N);

		return ConstructArray<T>(as_void, N);
	}


	template <typename T>
	inline void Delete(T* object, MemoryArenaBase* arena)
	{
		X_ASSERT_NOT_NULL(object);
		X_ASSERT_NOT_NULL(arena);

		// the object has been created using placement new, hence we need to call its destructor manually
		Destruct(object);
		arena->free(object);
	}

	template <typename T>
	inline void DeleteAndNull(T*& object, MemoryArenaBase* arena)
	{
		X_ASSERT_NOT_NULL(object);
		X_ASSERT_NOT_NULL(arena);

		// the object has been created using placement new, hence we need to call its destructor manually
		Destruct(object);
		arena->free(object);

		// null the pointer.
		object = nullptr;
	}

	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena)
	{
		X_ASSERT_NOT_NULL(ptr);
		X_ASSERT_NOT_NULL(arena);

		// delegate to the proper implementation based on T's type (POD or non-POD)
		DeleteArray(ptr, arena, compileTime::IntToType<compileTime::IsPOD<T>::Value>());
	}


	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena, PODType)
	{
		X_ASSERT_NOT_NULL(ptr);
		X_ASSERT_NOT_NULL(arena);

		// no destructors need to be called for POD types
		arena->free(ptr); 
	}


	template <typename T>
	inline void DeleteArray(T* ptr, MemoryArenaBase* arena, NonPODType)
	{
		X_ASSERT_NOT_NULL(ptr);
		X_ASSERT_NOT_NULL(arena);

		union
		{
			uint32_t* as_uint32_t;
			T* as_T;
		};

		// the user pointer points to the first instance, so go back 4 bytes and grab the number of instances
		as_T = ptr;
		const uint32_t N = as_uint32_t[-1];

		// properly destruct the instances
		DestructArray(as_T, N);

		arena->free(as_uint32_t - 1); 
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline T* Construct(void* where)
	{
		X_ASSERT_NOT_NULL(where);

		return new (where) T;
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline T* Construct(void* where, const T& what)
	{
		X_ASSERT_NOT_NULL(where);

		return new (where) T(what);
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline T* ConstructArray(void* where, size_t N)
	{
		X_ASSERT_NOT_NULL(where);

		T* as_T = union_cast<T*>(where);

		// construct instances using placement new
		const T* const onePastLast = as_T + N;
		while (as_T < onePastLast)
			Construct<T>(as_T++);

		return (as_T - N);
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline void Destruct(T* instance)
	{
		X_ASSERT_NOT_NULL(instance);

		// delegate to the proper implementation based on T's type (POD or not)
		Destruct(instance, compileTime::IntToType<compileTime::IsPOD<T>::Value>());
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline void Destruct(T* instance, PODType)
	{
		X_ASSERT_NOT_NULL(instance);

		// no destructor needs to be called for POD types
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline void Destruct(T* instance, NonPODType)
	{
		X_ASSERT_NOT_NULL(instance);

		instance->~T();
	}


	// ---------------------------------------------------------------------------------------------------------------------
	// ---------------------------------------------------------------------------------------------------------------------
	template <typename T>
	inline void DestructArray(T* instances, size_t N)
	{
		X_ASSERT_NOT_NULL(instances);

		// call the instances' destructors in reverse order
		for (size_t i=N; i>0; --i)
			Destruct(instances + i - 1);
	}
}
