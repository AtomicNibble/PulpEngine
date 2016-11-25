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

		SHADERLIB_EXPORT bool saveShader(const char* pPath, const XHWShader* pShader);
		SHADERLIB_EXPORT bool loadShader(const char* pPath, XHWShader* pShader);

	private:
		// returns true if we know the file has a diffrent crc32.
		// saves opening it.
		bool cacheNotValid(const char* pPath, uint32_t sourceCrc32) const;
		void updateCacheCrc(const char* pPath, uint32_t sourceCrc32);

	private:
		core::MemoryArenaBase* scratchArena_;
		// any point caching these?
		// maybe caching the hashes might be worth it.
		HashCacheMap cache_;
	};

} // namespace shader

X_NAMESPACE_END

#endif // !X_SHADER_BIN_H_
