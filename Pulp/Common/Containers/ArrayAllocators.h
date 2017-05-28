#pragma once


#ifndef X_CON_ARRAY_ALLOC_H
#define X_CON_ARRAY_ALLOC_H

X_NAMESPACE_BEGIN(core)

namespace growStrat
{

	class Linear
	{
	public:
		typedef size_t size_type;

		void setGranularity(size_type size);

	protected:
		size_type getAllocationSize(size_type currentCapacity) const {
			return currentCapacity + granularity_;
		}

	private:
		size_type granularity_;
	};

	template<size_t GrowSize>
	class FixedLinear
	{
	public:
		typedef size_t size_type;

	protected:
		size_type getAllocationSize(size_type currentCapacity) const {
			return currentCapacity + GrowSize;
		}
	};

	// fuck naming shit.
	class Expandy
	{
	public:
		typedef size_t size_type;

	protected:
		size_type getAllocationSize(size_type currentCapacity) const
		{
			if (currentCapacity == 0) {
				return 16;
			}

			// if it's big grow slower.
			if (currentCapacity > 1024 * 64) {
				return currentCapacity * 2;
			}

			// 1.5 grow.
			return (currentCapacity * 3 + 1) / 2;
		}

	};

} // namespace growStrat



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

	X_INLINE size_t getBaseAlignment(void) const {
		return alignment_;
	}


	X_INLINE T* allocate(size_t num) {
		X_ASSERT((alignment_ % X_ALIGN_OF(T)) == 0, "custom base alignment must be a multiple of type alignment")(alignment_, X_ALIGN_OF(T));
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, alignment_, 0, "Array", "T[]", X_SOURCE_INFO));
	}

	X_INLINE void free(void* pArr) {
		arena_->free(pArr);
	}

private:
	MemoryArenaBase* arena_;
	size_t alignment_;
};

template<typename T, size_t alignment>
class ArrayAlignedAllocatorFixed
{
public:
	X_INLINE ArrayAlignedAllocatorFixed(MemoryArenaBase* arena) :
		arena_(arena)
	{
		static_assert(alignment >= X_ALIGN_OF(T), "Fixed alignment don't satisfy type alignment");
	}

	X_INLINE void setArena(MemoryArenaBase* arena) {
		arena_ = arena;
	}
	X_INLINE MemoryArenaBase* getArena(void) const {
		return arena_;
	}

	X_INLINE size_t getBaseAlignment(void) const {
		return alignment;
	}

	X_INLINE T* allocate(size_t num) {
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, alignment, 0, "Array", "T[]", X_SOURCE_INFO));
	}

	X_INLINE void free(void* pArr) {
		arena_->free(pArr);
	}

private:
	MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#endif // X_CON_ARRAY_ALLOC_H
