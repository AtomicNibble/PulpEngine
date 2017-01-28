#pragma once


#ifndef X_SHADER_BIN_H_
#define X_SHADER_BIN_H_

#include <Time\CompressedStamps.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	class XHWShader;

	class ShaderBin
	{
		typedef core::HashMap<core::string, uint32_t> HashCacheMap;

	public:
		SHADERLIB_EXPORT ShaderBin(core::MemoryArenaBase* arena);
		SHADERLIB_EXPORT ~ShaderBin();

		// this is thread safe if saveShader and loadShader are not called for the same instance, must be diffrent instances.
		SHADERLIB_EXPORT bool saveShader(const XHWShader* pShader);
		SHADERLIB_EXPORT bool loadShader(XHWShader* pShader);

	private:
		// returns true if we know the file has a diffrent crc32.
		// saves opening it.
		bool cacheNotValid(core::Path<char>& path, uint32_t sourceCrc32) const;
		void updateCacheCrc(core::Path<char>& path, uint32_t sourceCrc32);

	private:
		void getShaderCompileDest(const XHWShader* pShader, core::Path<char>& destOut);


	private:
		core::MemoryArenaBase* scratchArena_;
		core::IFileSys* pFileSys_;

		// any point caching these?
		// maybe caching the hashes might be worth it.
		HashCacheMap cache_;

		mutable core::CriticalSection cs_;
	};

} // namespace shader

X_NAMESPACE_END

#endif // !X_SHADER_BIN_H_
