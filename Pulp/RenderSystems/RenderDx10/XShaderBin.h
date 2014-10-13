#pragma once


#ifndef X_SHADER_BIN_H_
#define X_SHADER_BIN_H_

#include "Time\CompressedStamps.h"

X_NAMESPACE_BEGIN(shader)

static const uint32_t X_SHADER_BIN_FOURCC = X_TAG('X','S','C','B');
static const uint32_t X_SHADER_BIN_VERSION = 1;


class XHWShader;

struct XShaderBinHeader
{
	uint32_t forcc;
	uint8_t version;
	uint8_t unused[3];
	uint32_t crc32;
	uint32_t sourceCRC32;
	uint32_t length;
	uint32_t compileFlags;
	core::dateTimeStampSmall modifed;

	X_INLINE const bool isValid(void) const {
		return forcc == X_SHADER_BIN_FOURCC;
	}
};


class XShaderBin
{
public:
	XShaderBin();
	~XShaderBin();


	bool saveShader(const char* path, uint32_t sourceCRC, uint32_t compileFlags, const char* pData, uint32_t len);
	bool loadShader(const char* path, uint32_t sourceCRC, ID3DBlob** pBlob);

};

X_NAMESPACE_END

#endif // !X_SHADER_BIN_H_