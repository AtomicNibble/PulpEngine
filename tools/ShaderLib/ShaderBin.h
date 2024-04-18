#pragma once

#ifndef X_SHADER_BIN_H_
#define X_SHADER_BIN_H_

#include <ICompression.h>

#include <Time\CompressedStamps.h>
#include <Containers\FixedHashTable.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class XHWShader;
    class SourceFile;

    class ShaderBin
    {
        typedef core::FixedHashTable<core::string, uint32_t> HashCacheMap;

    public:
        SHADERLIB_EXPORT ShaderBin(core::MemoryArenaBase* arena);
        SHADERLIB_EXPORT ~ShaderBin();

        // this is thread safe if saveShader and loadShader are not called for the same instance, must be different instances.
        SHADERLIB_EXPORT bool saveShader(const XHWShader* pShader, const SourceFile* pSource);
        SHADERLIB_EXPORT bool loadShader(XHWShader* pShader, const SourceFile* pSource);

        SHADERLIB_EXPORT bool clearBin(void);

        X_INLINE void setCompressionLvl(core::Compression::CompressLevel::Enum lvl);

    private:
        // returns true if we know the file has a different crc32.
        // saves opening it.
        bool cacheNotValid(const core::Path<char>& path, uint32_t sourceCrc32);
        void updateCacheCrc(const core::Path<char>& path, uint32_t sourceCrc32);

    private:
        void getShaderCompileDest(const XHWShader* pShader, core::Path<char>& destOut, bool createDir);

    private:
        core::MemoryArenaBase* scratchArena_;

        // any point caching these?
        // maybe caching the hashes might be worth it.
        HashCacheMap cache_;

        core::CriticalSection cs_;

        core::Compression::CompressLevel::Enum compLvl_;
    };

} // namespace shader

X_NAMESPACE_END

#include "ShaderBin.inl"

#endif // X_SHADER_BIN_H_
