#include "stdafx.h"
#include "ShaderBin.h"
#include "ShaderUtil.h"

#include <IFileSys.h>
#include <Hashing\crc32.h>

#include "HWShader.h"

#include <ICompression.h>
#include <Compression\LZ4.h>


X_NAMESPACE_BEGIN(render)

namespace shader
{

	namespace
	{
		X_DECLARE_FLAGS8(BinFileFlag)(
			COMPRESSED
		);

		typedef Flags8<BinFileFlag> BinFileFlags;

		struct ShaderBinHeader
		{
			static const uint32_t X_SHADER_BIN_FOURCC = X_TAG('X', 'S', 'C', 'B');
			static const uint32_t X_SHADER_BIN_VERSION = 1; // change this to force all shaders to be recompiled.


			uint32_t forcc;
			uint8_t version;
			// the version it was compiled against.
			uint8_t profileMajorVersion;
			uint8_t profileMinorVersion;
			BinFileFlags flags;
			uint32_t crc32;
			uint32_t sourceCRC32;
			uint32_t blobLength;
			uint32_t deflatedLength;
			uint32_t compileFlags;
			core::dateTimeStampSmall modifed;

			// i now save reflection info.
			uint8_t numInputParams;
			uint8_t numRenderTargets;
			uint8_t numSamplers;
			uint8_t numTextures;
			// 
			uint8_t numCBufs;
			uint8_t __pad[3];

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

		X_ENSURE_SIZE(ShaderBinHeader, 48);

	} // namespace

	ShaderBin::ShaderBin(core::MemoryArenaBase* arena) :
		scratchArena_(arena),
		cache_(arena, 32)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);

		pFileSys_ = gEnv->pFileSys;
	}

	ShaderBin::~ShaderBin()
	{

	}



	bool ShaderBin::saveShader(const XHWShader* pShader)
	{
		X_ASSERT_NOT_NULL(pShader);

		if (!pShader->isValid())
		{
			X_ERROR("Shader", "Failed to save compiled shader it's not valid.");
			return false;
		}
		if (pShader->getType() == ShaderType::Vertex && !pShader->isILFmtValid())
		{
			X_ERROR("Shader", "Failed to save compiled shader it's not valid.");
			return false;
		}

		const auto& byteCode = pShader->getShaderByteCode();
		const auto& cbuffers = pShader->getCBuffers();

		// for now every shader for a given type is compiled with same version.
		// if diffrent shaders have diffrent versions this will ned changing.
		auto profileVersion = Util::getProfileVersionForType(pShader->getType());

		ShaderBinHeader hdr;
		core::zero_object(hdr);
		hdr.forcc = ShaderBinHeader::X_SHADER_BIN_FOURCC;
		hdr.version = ShaderBinHeader::X_SHADER_BIN_VERSION;
		hdr.profileMajorVersion = profileVersion.first;
		hdr.profileMinorVersion = profileVersion.second;
		hdr.flags.Set(BinFileFlag::COMPRESSED);
		hdr.modifed = core::dateTimeStampSmall::systemDateTime();
		hdr.crc32 = gEnv->pCore->GetCrc32()->GetCRC32(byteCode.data(), byteCode.size());
		hdr.sourceCRC32 = pShader->getSourceCrc32();
		hdr.compileFlags = pShader->getD3DCompileFlags();
		hdr.blobLength = safe_static_cast<uint32_t>(byteCode.size());
		hdr.deflatedLength = 0;

		// shader reflection info.
		hdr.numInputParams = pShader->getNumInputParams();
		hdr.numRenderTargets = pShader->getNumRenderTargets();
		hdr.numSamplers = pShader->getNumSamplers();
		hdr.numTextures = pShader->getNumTextures();
		hdr.numCBufs = pShader->getNumConstantBuffers();

		hdr.techFlags = pShader->getTechFlags();
		hdr.type = pShader->getType();
		hdr.ILFmt = pShader->getILFormat();

		core::Path<char> path;
		getShaderCompileDest(pShader, path);

		// do the compression pre lock
		typedef core::Array<uint8_t> DataArr;

		DataArr compressed(scratchArena_);
		const DataArr* pData = &byteCode;

		if (hdr.flags.IsSet(BinFileFlag::COMPRESSED))
		{
			core::Compression::Compressor<core::Compression::LZ4> comp;

			if (!comp.deflate(scratchArena_, byteCode, compressed, core::Compression::CompressLevel::NORMAL))
			{
				X_ERROR("Shader", "Failed to defalte data");
				return false;
			}

			hdr.deflatedLength = safe_static_cast<uint32_t, size_t>(compressed.size());

			pData = &compressed;
		}

		// just lock while we save, keep it simple.
		// i don't think we'll see much contention.
		core::CriticalSection::ScopedLock lock(cs_);

		core::XFileScoped file;
		if (file.openFile(path.c_str(), core::fileMode::WRITE | core::fileMode::RECREATE))
		{
			X_ASSERT_NOT_NULL(pData);

			file.write(hdr);

			for (const auto& cb : cbuffers) {
				cb.SSave(file.GetFile());
			}

			if (file.write(pData->data(), pData->size()) != pData->size()) {
				return false;
			}
		}
		else
		{
			// file system will print path for us.
			X_ERROR("Shader", "failed to save compiled shader.");
			return false;
		}

		updateCacheCrc(lock, path, pShader->getSourceCrc32());
		return true;
	}


	bool ShaderBin::loadShader(XHWShader* pShader)
	{
		X_ASSERT_NOT_NULL(pShader);

		core::Path<char> path;
		getShaderCompileDest(pShader, path);

		if (cacheNotValid(path, pShader->getSourceCrc32())) {
			return false;
		}

		if (!pFileSys_->fileExists(path.c_str())) {
			X_LOG1("Shader", "no cache exsits for: \"%s\"", path.c_str());
			return false;
		}

		ShaderBinHeader hdr;
		core::zero_object(hdr);

		core::XFileScoped file;
		if (file.openFile(path.c_str(), core::fileMode::READ | core::fileMode::SHARE))
		{
			file.readObj(hdr);

			if (hdr.isValid())
			{
				if (hdr.version != ShaderBinHeader::X_SHADER_BIN_VERSION)
				{
					X_WARNING("Shader", "bin shader \"%s\" version is invalid. provided: %i, required: %i",
						path.c_str(), hdr.version, ShaderBinHeader::X_SHADER_BIN_VERSION);
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
				auto profileVersion = Util::getProfileVersionForType(pShader->getType());
				if (profileVersion.first != hdr.profileMajorVersion || profileVersion.second != hdr.profileMinorVersion)
				{
					X_WARNING("Shader", "bin shader is stale, compiled with diffrent shader mode: %" PRIu8 "_%" PRIu8
						" requested: %" PRIu8 "_%" PRIu8,
						hdr.profileMajorVersion, hdr.profileMinorVersion, profileVersion.first, profileVersion.second);
					return false;
				}

				// clear status, as we edit members.
				pShader->status_ = ShaderStatus::NotCompiled;


				auto& cbufs = pShader->getCBuffers();
				cbufs.resize(hdr.numCBufs, cbufs.getArena());

				// load bind vars.
				for (auto& cb : cbufs)
				{
					cb.SLoad(file.GetFile());
				}

				pShader->bytecode_.resize(hdr.blobLength);

				if (hdr.flags.IsSet(BinFileFlag::COMPRESSED))
				{
					if (hdr.deflatedLength < 1) {
						X_ERROR("Shader", "Failed to read shader byte code");
						return false;
					}

					core::Compression::Compressor<core::Compression::LZ4> comp;
					core::Array<uint8_t> compressed(scratchArena_, hdr.deflatedLength);

					if (file.read(compressed.data(), hdr.deflatedLength) != hdr.deflatedLength) {
						X_ERROR("Shader", "Failed to read shader byte code");
						return false;
					}

					if (!comp.inflate(scratchArena_, compressed, pShader->bytecode_))
					{
						X_ERROR("Shader", "Failed to inflate data");
						return false;
					}
				}
				else
				{
					if (file.read(pShader->bytecode_.data(), hdr.blobLength) != hdr.blobLength) {
						X_ERROR("Shader", "Failed to read shader byte code");
						return false;
					}
				}

				pShader->numInputParams_ = hdr.numInputParams;
				pShader->numRenderTargets_ = hdr.numRenderTargets;
				pShader->numSamplers_ = hdr.numSamplers;
				pShader->numTextures_ = hdr.numTextures;
				X_ASSERT(pShader->getNumConstantBuffers() == hdr.numCBufs, "Cbuffer count not correct")();

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


	bool ShaderBin::cacheNotValid(core::Path<char>& path, uint32_t sourceCrc32) const
	{
		core::CriticalSection::ScopedLock lock(cs_);

		auto it = cache_.find(X_CONST_STRING(path.c_str()));
		if (it != cache_.end()) {
			if (it->second != sourceCrc32) {
				// we have a cache file, but it was compiled with source that had diffrent crc.
				return true;
			}
		}
		return false;
	}

	void ShaderBin::updateCacheCrc(core::Path<char>& path, uint32_t sourceCrc32)
	{
		core::CriticalSection::ScopedLock lock(cs_);

		updateCacheCrc(lock, path, sourceCrc32);
	}

	void ShaderBin::updateCacheCrc(const core::CriticalSection::ScopedLock& lock, core::Path<char>& path, uint32_t sourceCrc32)
	{
		cache_.insert(std::make_pair(core::string(path.c_str()), sourceCrc32));
	}

	void ShaderBin::getShaderCompileDest(const XHWShader* pShader, core::Path<char>& destOut)
	{
		destOut.clear();
		destOut.appendFmt("shaders/compiled/%s.fxcb", pShader->getName().c_str());

		// make sure the directory is created.
		pFileSys_->createDirectoryTree(destOut.c_str());
	}



} // namespace shader


X_NAMESPACE_END