#include "stdafx.h"
#include "XShaderBin.h"

#include <IFileSys.h>

#include <Hashing\crc32.h>

#include <D3Dcompiler.h>

#include "Dx10Shader.h"

X_NAMESPACE_BEGIN(shader)



XShaderBin::XShaderBin()
{

}

XShaderBin::~XShaderBin()
{

}



bool XShaderBin::saveShader(const char* path, uint32_t sourceCRC,  
	const XHWShader_Dx10* pShader)
{
	X_ASSERT_NOT_NULL(path);
	X_ASSERT_NOT_NULL(pShader);

	if (!pShader->isValid())
	{
		X_ERROR("Shader", "Failed to save compiled shader it's not valid.");
		return false;
	}

	ID3DBlob* pBlob = pShader->getshaderBlob();
	const char* pData;
	uint32_t blobLen;

	pData = reinterpret_cast<char*>(pBlob->GetBufferPointer());
	blobLen = safe_static_cast<uint32_t, size_t>(pBlob->GetBufferSize());
	
	const core::Array<XShaderParam>& bindVars = pShader->getBindVars();

	XShaderBinHeader hdr;
	core::zero_object(hdr);
	hdr.forcc = X_SHADER_BIN_FOURCC;
	hdr.version = X_SHADER_BIN_VERSION;
	hdr.modifed = core::dateTimeStampSmall::systemDateTime();
	hdr.crc32 = gEnv->pCore->GetCrc32()->GetCRC32(pData, blobLen);
	hdr.sourceCRC32 = sourceCRC;
	hdr.compileFlags = pShader->getD3DCompileFlags();
	hdr.blobLength = blobLen;

	// shader reflection info.
	hdr.numBindVars = safe_static_cast<uint32_t, size_t>(bindVars.size());
	hdr.numSamplers = pShader->getNumSamplers();
	hdr.numConstBuffers = pShader->getNumConstantBuffers();
	hdr.numInputParams = pShader->getNumInputParams();
	hdr.numRenderTargets = pShader->getNumRenderTargets();

	hdr.techFlags = pShader->getTechFlags();
	hdr.type = pShader->getType();
	hdr.ILFmt = pShader->getILFormat();

	core::XFileScoped file;
	if (file.openFile(path, core::fileMode::WRITE | core::fileMode::RECREATE))
	{
		file.write(hdr);

		for (auto& bind : bindVars)
		{
			file.writeString(bind.name);
			file.write(bind.nameHash);
			file.write(bind.flags);
			file.write(bind.type);
			file.write(bind.bind);
			file.write(bind.constBufferSlot);
			file.write(bind.numParameters);
		}

		file.write(pData, blobLen);
	}
	else
	{
		// file system will print path for us.
		X_ERROR("Shader", "failed to save compiled shader.");
		return false;
	}

	return true;
}


bool XShaderBin::loadShader(const char* path, uint32_t sourceCRC, XHWShader_Dx10* pShader)
{
	X_ASSERT_NOT_NULL(path);
	X_ASSERT_NOT_NULL(pShader);

	if (!gEnv->pFileSys->fileExists(path)) {
		X_LOG1("Shader", "no cache exsits for: \"%s\"", path);
		return false;
	}


	ID3DBlob* pBlob = nullptr;
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

			if (hdr.blobLength < 1)
			{
				X_WARNING("Shader", "bin shader has invalid blob length");
				return false;
			}

			if (hdr.sourceCRC32 != sourceCRC)
			{
				X_WARNING("Shader", "bin shader is stale, recompile needed.");
				return false;
			}


			core::Array<XShaderParam> bindvars(g_rendererArena);
			bindvars.reserve(hdr.numBindVars);

			// load bind vars.
			uint32_t i;
			for (i = 0; i < hdr.numBindVars; i++)
			{
				XShaderParam bind;

				file.readString(bind.name);
				file.read(bind.nameHash);
				file.read(bind.flags);
				file.read(bind.type);
				file.read(bind.bind);
				file.read(bind.constBufferSlot);
				file.read(bind.numParameters);

				bindvars.append(bind);
			}

			// load the shader blob.
			if (SUCCEEDED(D3DCreateBlob(hdr.blobLength, &pBlob)))
			{
				if (file.read(pBlob->GetBufferPointer(),
					(uint32_t)pBlob->GetBufferSize()))
				{
					// set the values.
					pShader->setBlob(pBlob);
					pShader->setBindVars(bindvars);


					pShader->numSamplers_ = hdr.numSamplers;
					pShader->numConstBuffers_ = hdr.numConstBuffers;
					pShader->numInputParams_ = hdr.numInputParams;
					pShader->numRenderTargets_ = hdr.numRenderTargets;

					pShader->techFlags_ = hdr.techFlags;
					pShader->IlFmt_ = hdr.ILFmt;

					// type should already be set.
					// so we just use it as a sanity check.
					if (pShader->getType() != hdr.type)
					{
						X_WARNING("Shader", "Shader type is diffrent to that of the cache file for: \"%s\"",
							pShader->getName());
					}

					return true;
				}
				 
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