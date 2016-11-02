#pragma once


#ifndef X_CON_ARRAY_ALLOC_H
#define X_CON_ARRAY_ALLOC_H

X_NAMESPACE_BEGIN(core)

template<typename T>
class ArrayAllocator
{
public:
	X_INLINE ArrayAllocator(MemoryArenaBase* arena) :
		arena_(arena)
	{}

	X_INLINE void setArena(MemoryArenaBase* arena) {
		arena_ = arena;
	}
	X_INLINE MemoryArenaBase* getArena(void) const {
		return arena_;
	}

	X_INLINE T* allocate(size_t num) {
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, X_ALIGN_OF(T), 0, "Array", "T[]", X_SOURCE_INFO));
	}

	X_INLINE void free(void* pArr) {
		arena_->free(pArr);
	}

private:
	MemoryArenaBase* arena_;
};

template<typename T>
class ArrayAlignedAllocator
{
public:
	X_INLINE ArrayAlignedAllocator(MemoryArenaBase* arena) :
		arena_(arena),
		alignment_(X_ALIGN_OF(T))
	{}

	X_INLINE void setArena(MemoryArenaBase* arena) {
		arena_ = arena;
	}
	X_INLINE MemoryArenaBase* getArena(void) const {
		return arena_;
	}

	X_INLINE void setBaseAlignment(size_t alignment) {
		alignment_ = alignment;
	}

	X_INLINE size_t setBaseAlignment(void) const {
		return alignment_;
	}


	X_INLINE T* allocate(size_t num) {
		X_ASSERT((alignment_ == X_ALIGN_OF(T)) == 0, "custom base alignment but be a multiple of type alignment")(alignment_, X_ALIGN_OF(T));
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, alignment_, 0, "Array", "T[]", X_SOURCE_INFO));
	}

	X_INLINE void free(void* pArr) {
		arena_->free(pArr);
	}

private:
	MemoryArenaBase* arena_;
	size_t alignment_;
};


X_NAMESPACE_END

#endif // X_CON_ARRAY_ALLOC_H
