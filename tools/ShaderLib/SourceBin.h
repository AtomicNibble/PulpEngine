#pragma once

#include <IShader.h>

#include <Containers\FixedHashTable.h>

#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Threading\CriticalSection.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class SourceFile;

    class SourceBin
    {
        typedef core::FixedHashTable<core::string, SourceFile*> ShaderSourceMap;
        typedef core::Array<SourceFile*> SourceRefArr;
        typedef core::Array<uint8_t> ByteArr;

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
            >
            PoolArena;

    public:
        struct SourceInfo
        {
            core::string name;
            int32_t line;
        };

    public:
        SHADERLIB_EXPORT SourceBin(core::MemoryArenaBase* arena);
        SHADERLIB_EXPORT ~SourceBin();

        SHADERLIB_EXPORT void free(void);

        // returns the source of a shader with all it's includes merged.
        SHADERLIB_EXPORT bool getMergedSource(const SourceFile* pSourceFile, ByteArr& strOut);
        SHADERLIB_EXPORT SourceInfo getSourceInfoForMergedLine(const SourceFile* pSourceFile, size_t line);

        SHADERLIB_EXPORT SourceFile* loadRawSourceFile(const core::string& name, bool reload);
        SHADERLIB_EXPORT SourceFile* sourceForName(const core::string& name);

        SHADERLIB_EXPORT void listShaderSources(core::string_view searchPattern);

    private:
        void parseIncludesAndPrePro_r(SourceFile* file, SourceRefArr& includedFiles,
            bool reload = false);

    private:
        core::MemoryArenaBase* arena_;
        core::Crc32* pCrc32_;

        // allocator for source objects.
        core::HeapArea sourcePoolHeap_;
        core::PoolAllocator sourcePoolAllocator_;
        PoolArena sourcePoolArena_;

        core::CriticalSection cs_;
        ShaderSourceMap source_;
    };

} // namespace shader

X_NAMESPACE_END
