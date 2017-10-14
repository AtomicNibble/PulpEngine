#include "stdafx.h"
#include "Allocators.h"



#include <Memory\SimpleMemoryArena.h>


namespace AK
{
	namespace
	{
		class VirtualMemStatsMemoryArena : public core::MemoryArenaBase
		{
		public:
			static const bool IS_THREAD_SAFE = true;

		public:
			VirtualMemStatsMemoryArena() {

				statistics_.arenaName_ = "SndVirtualMem";
				statistics_.arenaType_ = "VirtualMem";
				statistics_.threadPolicyType_ = "";
				statistics_.boundsCheckingPolicyType_ = "";
				statistics_.memoryTrackingPolicyType_ = "";
				statistics_.memoryTaggingPolicyType_ = "";
				statistics_.trackingOverhead_ = 0;
				statistics_.boundsCheckingOverhead_ = 0;

				auto& allocStats = statistics_.allocatorStatistics_;
				allocStats.type_ = "VirtualMem";
				allocStats.Clear();
			}

			X_INLINE void* VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
			{
#if X_ENABLE_MEMORY_ARENA_STATISTICS

				auto& allocStats = statistics_.allocatorStatistics_;
				++allocStats.allocationCount_;

				allocStats.allocationCountMax_ = core::Max(allocStats.allocationCountMax_, allocStats.allocationCount_);

				if (core::bitUtil::IsBitFlagSet(flAllocationType, MEM_COMMIT))
				{
					allocStats.physicalMemoryAllocated_ += dwSize;
					allocStats.physicalMemoryUsed_ += dwSize;
					allocStats.physicalMemoryAllocatedMax_ = core::Max(allocStats.physicalMemoryAllocatedMax_, allocStats.physicalMemoryAllocated_);
					allocStats.physicalMemoryUsedMax_ = core::Max(allocStats.physicalMemoryUsedMax_, allocStats.physicalMemoryUsed_);

					// if we are commiting reservered mem don't double count.
					if (!lpAddress) {
						allocStats.virtualMemoryReserved_ += dwSize;
					}
				}
				else
				{
					allocStats.virtualMemoryReserved_ += dwSize;
				}
#endif

				return ::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);
			}


			BOOL VirtualFree(void* in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
			{
#if X_ENABLE_MEMORY_ARENA_STATISTICS

				auto& allocStats = statistics_.allocatorStatistics_;
				--allocStats.allocationCount_;

				if (core::bitUtil::IsBitFlagSet(in_dwFreeType, MEM_DECOMMIT))
				{
					allocStats.virtualMemoryReserved_ -= in_size;
				}
				else
				{
					allocStats.physicalMemoryAllocated_ -= in_size;
					allocStats.physicalMemoryUsed_ -= in_size;
					allocStats.virtualMemoryReserved_ -= in_size;
				}
#endif

				return ::VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
			}

		private:

			/// Allocates raw memory that satisfies the alignment requirements.
			virtual void* allocate(size_t, size_t, size_t
				X_MEM_HUMAN_IDS_CB(const char*) X_MEM_HUMAN_IDS_CB(const char*) X_SOURCE_INFO_MEM_CB(const core::SourceInfo&)) X_FINAL {

				X_ASSERT_UNREACHABLE();
				return nullptr;
			}

			/// Frees memory previously allocated by Allocate().
			virtual void free(void* ptr) X_FINAL {
				X_UNUSED(ptr);
				X_ASSERT_UNREACHABLE();
			}

			virtual void free(void* ptr, size_t size) X_FINAL {
				X_UNUSED(ptr, size);
				X_ASSERT_UNREACHABLE();
			}

			virtual size_t getSize(void* ptr) X_FINAL {
				X_UNUSED(ptr);
				X_ASSERT_UNREACHABLE();
				return 0;
			}

			virtual size_t usableSize(void* ptr) const X_FINAL {
				X_UNUSED(ptr);
				X_ASSERT_UNREACHABLE();
				return 0;
			}

			virtual core::MemoryArenaStatistics getStatistics(void) const X_FINAL {
				return statistics_;
			}

			virtual core::MemoryAllocatorStatistics getAllocatorStatistics(bool children) const X_FINAL {
				X_UNUSED(children);
				return statistics_.allocatorStatistics_;
			}

			virtual bool isThreadSafe(void) const X_FINAL {
				return true;
			}

			static inline size_t getMemoryRequirement(size_t size) {
				X_UNUSED(size);
				return 0;
			}
			static inline size_t getMemoryAlignmentRequirement(size_t alignment) {
				X_UNUSED(alignment);
				return 0;
			}
			static inline size_t getMemoryOffsetRequirement(void) {
				return 0;
			}

		private:
			X_NO_COPY(VirtualMemStatsMemoryArena);
			X_NO_ASSIGN(VirtualMemStatsMemoryArena);

			core::MemoryArenaStatistics statistics_;
		};

		VirtualMemStatsMemoryArena virtualAllocArena;
	}


	core::MallocFreeAllocator akAlloca;
	core::SimpleMemoryArena<core::MallocFreeAllocator> akArena(&akAlloca, "AKArena");

	// these are unresolved symbols in ak, so they get use just by been defined.
	void* AllocHook(size_t in_size)
	{
#if X_ENABLE_MEMORY_SOURCE_INFO
		const X_NAMESPACE(core)::SourceInfo sourceInfo("sound", __FILE__, __LINE__, __FUNCTION__, __FUNCSIG__);
#endif

		return akArena.allocate(in_size, 1, 0 X_MEM_IDS("SndAlloc", "uint8_t") X_SOURCE_INFO_MEM_CB(sourceInfo));
	}

	void FreeHook(void * in_ptr)
	{
		akArena.free(in_ptr);
	}


	// i just want to track stats, so we need like a adhock area that tracks virtual allocs.
	void* VirtualAllocHook(
		void* in_pMemAddress,
		size_t in_size,
		DWORD in_dwAllocationType,
		DWORD in_dwProtect
	)
	{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
		return virtualAllocArena.VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
#else
		return ::VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
#endif
	}

	void VirtualFreeHook(
		void* in_pMemAddress,
		size_t in_size,
		DWORD in_dwFreeType
	)
	{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
		virtualAllocArena.VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
#else
		::VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
#endif
	}

	void akAssertHook(const char* pszExpression, const char* pszFileName, int lineNumber)
	{
#if X_ENABLE_ASSERTIONS
		core::SourceInfo sourceInfo("SoundSys", pszFileName, lineNumber, "", "");
		core::Assert(sourceInfo, "Assertion \"%s\" failed.", pszExpression)
			.Variable("FileName", pszFileName)
			.Variable("LineNumber", lineNumber);

#else
		X_ERROR("SoundSys", "Sound system threw a assert: Exp: \"%s\" file: \"%s\" line: \"%s\"",
			pszExpression, pszFileName, lineNumber);
#endif

		X_BREAKPOINT;
	}

}


X_NAMESPACE_BEGIN(sound)


AllocatorHooks::AllocatorHooks()
{
	// link to arena tree.
	g_SoundArena->addChildArena(&AK::akArena);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	g_SoundArena->addChildArena(&AK::virtualAllocArena);
#endif

}


X_NAMESPACE_END