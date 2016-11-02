#include "stdafx.h"
#include "ShaderBin.h"

#include <IFileSys.h>
#include <Hashing\crc32.h>

#include "HWShader.h"


X_NAMESPACE_BEGIN(render)

namespace shader
{

	namespace
	{

		struct ShaderBinHeader
		{
			static const uint32_t X_SHADER_BIN_FOURCC = X_TAG('X', 'S', 'C', 'B');
			static const uint32_t X_SHADER_BIN_VERSION = 1; // change this to force all shaders to be recompiled.


			uint32_t forcc;
			uint8_t version;
			// the version it was compiled against.
			uint8_t profileMajorVersion;		
			uint8_t profileMinorVersion;		
			uint8_t unused[1];
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

			// 4
			TechFlags techFlags;
			// 4
			ShaderType::Enum type;
			InputLayoutFormat::Enum ILFmt;
			uint8_t _pad[2];


			X_INLINE const bool isValid(void) const {
				return forcc == X_SHADER_BIN_FOURCC;
			}
		};

		X_ENSURE_SIZE(ShaderBinHeader, 56);

	} // namespace

ShaderBin::ShaderBin(core::MemoryArenaBase* arena) :
	cache_(arena, 32)
{

}

ShaderBin::~ShaderBin()
{

}



bool ShaderBin::saveShader(const char* pPath, const XHWShader* pShader)
{
	X_ASSERT_NOT_NULL(pPath);
	X_ASSERT_NOT_NULL(pShader);

	if (!pShader->isValid() || !pShader->isILFmtValid())
	{
		X_ERROR("Shader", "Failed to save compiled shader it's not valid.");
		return false;
	}

	const auto& byteCode = pShader->getShaderByteCode();
	const auto& bindVars = pShader->getBindVars();

	// for now every shader for a given type is compiled with same version.
	// if diffrent shaders have diffrent versions this will ned changing.
	auto profileVersion = XHWShader::getProfileVersionForType(pShader->getType());

	ShaderBinHeader hdr;
	core::zero_object(hdr);
	hdr.forcc = ShaderBinHeader::X_SHADER_BIN_FOURCC;
	hdr.version = ShaderBinHeader::X_SHADER_BIN_VERSION;
	hdr.profileMajorVersion = profileVersion.first;
	hdr.profileMinorVersion = profileVersion.second;
	hdr.modifed = core::dateTimeStampSmall::systemDateTime();
	hdr.crc32 = gEnv->pCore->GetCrc32()->GetCRC32(byteCode.data(), byteCode.size());
	hdr.sourceCRC32 = pShader->getSourceCrc32();
	hdr.compileFlags = pShader->getD3DCompileFlags();
	hdr.blobLength = safe_static_cast<uint32_t, size_t>(byteCode.size());

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
	if (file.openFile(pPath, core::fileMode::WRITE | core::fileMode::RECREATE))
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

		if (file.write(byteCode.data(), byteCode.size()) != byteCode.size()) {
			return false;
		}
	}
	else
	{
		// file system will print path for us.
		X_ERROR("Shader", "failed to save compiled shader.");
		return false;
	}

	updateCacheCrc(pPath, pShader->getSourceCrc32());
	return true;
}


bool ShaderBin::loadShader(const char* pPath, XHWShader* pShader)
{
	X_ASSERT_NOT_NULL(pPath);
	X_ASSERT_NOT_NULL(pShader);

	if (cacheNotValid(pPath, pShader->getSourceCrc32())) {
		return false;
	}

	if (!gEnv->pFileSys->fileExists(pPath)) {
		X_LOG1("Shader", "no cache exsits for: \"%s\"", pPath);
		return false;
	}

	ShaderBinHeader hdr;
	core::zero_object(hdr);

	core::XFileScoped file;
	if (file.openFile(pPath, core::fileMode::READ))
	{
		file.readObj(hdr);

		if (hdr.isValid())
		{
			if (hdr.version != ShaderBinHeader::X_SHADER_BIN_VERSION)
			{
				X_WARNING("Shader", "bin shader \"%s\" version is invalid. provided: %i, required: %i",
					pPath, hdr.version, ShaderBinHeader::X_SHADER_BIN_VERSION);
				return false;
			}

			if (hdr.blobLength < 1)
			{
				X_WARNING("Shader", "bin shader has invalid blob length");
				return false;
			}

			if (hdr.sourceCRC32 != pShader->getSourceCrc32())
			{
				X_WARNING("Shader", "bin shader is stale, recompile needed.");
				return false;
			}

			// validate the profile version.
			auto profileVersion = XHWShader::getProfileVersionForType(pShader->getType());
			if (profileVersion.first != hdr.profileMajorVersion || profileVersion.second != hdr.profileMinorVersion)
			{
				X_WARNING("Shader", "bin shader is stale, compiled with diffrent shader mode: %" PRIu8 "_%" PRIu8
					" requested: %" PRIu8 "_%" PRIu8,
					hdr.profileMajorVersion, hdr.profileMinorVersion, profileVersion.first, profileVersion.second);
				return false;
			}

			// clear status, as we edit members.
			pShader->status_ = ShaderStatus::NotCompiled;


			auto& bindvars = pShader->bindVars_; 
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

			int32_t maxVecs[3] = {};
			for (i = 0; i < bindvars.size(); i++)
			{
				XShaderParam* pB = &bindvars[i];

				// set max slots
				if (pB->constBufferSlot < 3)
				{
					maxVecs[pB->constBufferSlot] = core::Max(pB->bind +
						pB->numParameters, maxVecs[pB->constBufferSlot]);
				}
			}

			pShader->bytecode_.resize(hdr.blobLength);

			if (file.read(pShader->bytecode_.data(), hdr.blobLength) != hdr.blobLength) {
				X_ERROR("Shader", "Failed to read shader byte code");
				return false;
			}

			std::memcpy(pShader->maxVecs_, maxVecs, sizeof(maxVecs));
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

			// ready to use.
			pShader->status_ = ShaderStatus::ReadyToRock;
			return true;
		}
	}
	return false;
}


bool ShaderBin::cacheNotValid(const char* pPath, uint32_t sourceCrc32) const
{
	auto it = cache_.find(X_CONST_STRING(pPath));
	if (it != cache_.end()) {
		if (it->second != sourceCrc32) {
			// we have a cache file, but it was compiled with source that had diffrent crc.
			return true;
		}
	}
	return false;
}

void ShaderBin::updateCacheCrc(const char* pPath, uint32_t sourceCrc32)
{
	cache_.insert(std::make_pair(core::string(pPath), sourceCrc32));
}

} // namespace shader


X_NAMESPACE_END