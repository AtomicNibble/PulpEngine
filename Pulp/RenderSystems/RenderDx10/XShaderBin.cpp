#include "stdafx.h"
#include "XShaderBin.h"

#include <IFileSys.h>

#include <Hashing\crc32.h>

#include <D3Dcompiler.h>

X_NAMESPACE_BEGIN(shader)



XShaderBin::XShaderBin()
{

}

XShaderBin::~XShaderBin()
{

}


bool XShaderBin::saveShader(const char* path, uint32_t sourceCRC, uint32_t flags, const char* pData, uint32_t len)
{	
	X_ASSERT_NOT_NULL(path);
	X_ASSERT_NOT_NULL(pData);

	XShaderBinHeader hdr;
	core::zero_object(hdr);
	hdr.forcc = X_SHADER_BIN_FOURCC;
	hdr.version = X_SHADER_BIN_VERSION;
	hdr.modifed = core::dateTimeStampSmall::systemDateTime();
	hdr.crc32 = gEnv->pCore->GetCrc32()->GetCRC32(pData, len);
	hdr.sourceCRC32 = sourceCRC;
	hdr.compileFlags = flags;
	hdr.length = len;

	core::XFileScoped file;
	if (file.openFile(path, core::fileMode::WRITE | core::fileMode::RECREATE))
	{
		file.write(hdr);
		file.write(pData, len);
	}
	else
	{
		// file system will print path for us.
		X_ERROR("Shader", "failed to save compiled shader.");
		return false;
	}

	return true;
}


bool XShaderBin::loadShader(const char* path, uint32_t sourceCRC, ID3DBlob** pBlob)
{
	X_ASSERT_NOT_NULL(path);

	XShaderBinHeader hdr;
	core::zero_object(hdr);

	core::XFileScoped file;
	if (file.openFile(path, core::fileMode::READ))
	{
		file.readObj(hdr);

		if (hdr.isValid())
		{
			if (hdr.version != X_SHADER_BIN_VERSION)
			{
				X_WARNING("Shader", "bin shader \"%s\" version is invalid. provided: %i, required: %i",
					path, hdr.version, X_SHADER_BIN_VERSION);
				return false;
			}

			if (hdr.length < 1)
			{
				X_WARNING("Shader", "bin shader has invalid length");
				return false;
			}

			if (hdr.sourceCRC32 != sourceCRC)
			{
				X_WARNING("Shader", "bin shader is stale, recompile needed.");
				return false;
			}

			// load the shader.
			if (SUCCEEDED(D3DCreateBlob(hdr.length, pBlob)))
			{
				if(file.read(pBlob[0]->GetBufferPointer(), (uint32_t)pBlob[0]->GetBufferSize()))
					return true;
				 
				X_ERROR("ShaderBin", "failed to read shader from bin");
			}
			else
			{
				X_ERROR("ShaderBin", "failed to create blob");
			}
		}
	}
	return false;
}




X_NAMESPACE_END