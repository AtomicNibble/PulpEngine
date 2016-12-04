#pragma once

#include <IShader.h>

#include <Containers\HashMap.h>

#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{

	class SourceFile;

	class SourceBin
	{
		typedef core::HashMap<core::string, SourceFile*> ShaderSourceMap;
		typedef core::Array<SourceFile*> SourceRefArr;

		// Shader Source
		typedef core::MemoryArena<
			core::PoolAllocator,
			core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
			core::SimpleBoundsChecking,
			core::SimpleMemoryTracking,
			core::SimpleMemoryTagging
#else
			core::NoBoundsChecking,
			core::NoMemoryTracking,
			core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
		> PoolArena;

	public:
		SHADERLIB_EXPORT SourceBin(core::MemoryArenaBase* arena);

		SHADERLIB_EXPORT void free(void);

		// returns the source of a shader with all it's includes merged.
		SHADERLIB_EXPORT bool getMergedSource(const core::string& name, core::string& strOut);

		SHADERLIB_EXPORT SourceFile* loadRawSourceFile(const char* pName, bool reload = false);
		SHADERLIB_EXPORT SourceFile* sourceForName(const char* pName);

		SHADERLIB_EXPORT void listShaderSources(const char* pSearchPatten);

	private:
		void parseIncludesAndPrePro_r(SourceFile* file, SourceRefArr& includedFiles,
			bool reload = false);


	private:
		core::MemoryArenaBase* arena_;
		core::Crc32* pCrc32_;

		// allocator for source objects.
		core::HeapArea      sourcePoolHeap_;
		core::PoolAllocator sourcePoolAllocator_;
		PoolArena			sourcePoolArena_;


		ShaderSourceMap source_;
	};


} // namespace shader

X_NAMESPACE_END

