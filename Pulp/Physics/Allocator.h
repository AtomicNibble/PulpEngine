#pragma once


X_NAMESPACE_BEGIN(physics)


class PhysxArenaAllocator : public physx::PxAllocatorCallback
{
public:
	PhysxArenaAllocator(core::MemoryArenaBase* arena) :
		arena_(arena) {}

public:
	X_INLINE void* allocate(size_t size, const char* typeName, const char* filename, int line) X_FINAL
	{
		core::SourceInfo srcInfo("Physx", filename, line, "-", "-");
		return arena_->allocate(size, 16, 0, "Physx", typeName, srcInfo);
	}

	X_INLINE void deallocate(void* ptr) X_FINAL
	{
		if (ptr) {
			arena_->free(ptr);
		}
	}

private:
	core::MemoryArenaBase* arena_;
};



X_NAMESPACE_END