#pragma once


#ifndef X_SHADER_BIN_H_
#define X_SHADER_BIN_H_

#include "Time\CompressedStamps.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{

	static const uint32_t X_SHADER_BIN_FOURCC = X_TAG('X', 'S', 'C', 'B');
	static const uint32_t X_SHADER_BIN_VERSION = 3; // change this to force all shaders to be recompiled.


	class XHWShader;

	struct ShaderBinHeader
	{
		uint32_t forcc;
		uint8_t version;
		uint8_t unused[3];
		uint32_t crc32;
		uint32_t sourceCRC32;
		uint32_t blobLength;
		uint32_t compileFlags;
		core::dateTimeStampSmall modifed;

		// i now save reflection info.
		uint32_t numBindVars;
		uint32_t numSamplers;
		uint32_t numConstBuffers;
		uint32_t numInputParams;
		uint32_t numRenderTargets;

		TechFlags techFlags;
		ShaderType::Enum type;
		InputLayoutFormat::Enum ILFmt;

		X_INLINE const bool isValid(void) const {
			return forcc == X_SHADER_BIN_FOURCC;
		}
	};


	class ShaderBin
	{
	public:
		ShaderBin();
		~ShaderBin();


		bool saveShader(const char* pPath, uint32_t sourceCRC, const XHWShader* pShader);
		bool loadShader(const char* pPath, uint32_t sourceCRC, XHWShader* pShader);

	private:
		// any point caching these?
		// maybe caching the hashes might be worth it.
	};

} // namespace shader

X_NAMESPACE_END

#endif // !X_SHADER_BIN_H_