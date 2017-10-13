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

		Linear() {
			granularity_ = 16;
		}

		void setGranularity(size_type granularity) {
			granularity_ = granularity;
		}
		size_t granularity(void) const {
			return granularity_;
		}

	protected:
		X_INLINE size_type getAllocationSize(size_type currentCapacity, size_t requestedSize) const {
			X_UNUSED(currentCapacity);
			return core::bitUtil::RoundUpToMultiple(requestedSize, granularity_);
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
		X_INLINE static size_type getAllocationSize(size_type currentCapacity, size_t requestedSize) {
			return core::Max(currentCapacity + GrowSize, requestedSize);
		}
	};

	// fuck naming shit.
	class Multiply
	{
	public:
		typedef size_t size_type;

	protected:
		X_INLINE static size_type getAllocationSize(size_type currentCapacity, size_t requestedSize) 
		{
			X_UNUSED(requestedSize);

			if (currentCapacity == 0) {
				return core::Max(16_sz, requestedSize);
			}

			// if it's big grow faster?
			if (currentCapacity > 1024 * 64) {
				return core::Max(currentCapacity * 2, requestedSize);
			}

			// 1.5 grow.
			return core::Max((currentCapacity * 3 + 1) / 2, requestedSize);
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
		return static_cast<T*>(arena_->allocate(sizeof(T)*num, X_ALIGN_OF(T), 0
			X_MEM_IDS("Array", "T[]") X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO)));
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
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, alignment_, 0
			X_MEM_IDS("Array", "T[]") X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO)));
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
		return  static_cast<T*>(arena_->allocate(sizeof(T)*num, alignment, 0
			X_MEM_IDS("Array", "T[]") X_SOURCE_INFO_MEM_CB(X_SOURCE_INFO)));
	}

	X_INLINE void free(void* pArr) {
		arena_->free(pArr);
	}

private:
	MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#endif // X_CON_ARRAY_ALLOC_H
