#include "stdafx.h"
#include "ShaderManager.h"
#include "ShaderPermatation.h"

#include <Hashing\crc32.h>
#include <Hashing\sha1.h>
#include <String\Lexer.h>
#include <String\StringHash.h>
#include <String\AssetName.h>

#include <Util\UniquePointer.h>
#include <Threading\JobSystem2.h>
#include <Threading\UniqueLock.h>
#include <Threading\ScopedLock.h>

#include <IConsole.h>
#include <IFileSys.h>

#include "ShaderSourceTypes.h"
#include "HWShader.h"
#include "ShaderUtil.h"

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{

		template<typename TFlags>
		void AppendFlagTillEqual(const Flags<TFlags>& srcflags, Flags<TFlags>& dest)
		{
			if (srcflags.IsAnySet() && srcflags.ToInt() != dest.ToInt())
			{
				for (size_t i = 0; i < TFlags::FLAG_COUNT; i++)
				{
					TFlags::Enum flag = static_cast<TFlags::Enum>(1 << i);
					if (srcflags.IsSet(flag) && !dest.IsSet(flag)) {
						dest.Set(flag);
						return;
					}
				}
			}
		}

	} // namespace 

	XShaderManager::XShaderManager(core::MemoryArenaBase* arena) :
		arena_(arena),
		shaderBin_(arena),
		sourceBin_(arena),
		hwShaders_(arena, sizeof(HWShaderResource), X_ALIGN_OF(HWShaderResource), "HWShaderPool"),
		permHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				PoolArena::getMemoryRequirement(sizeof(ShaderPermatation)) * MAX_SHADER_PERMS,
				core::VirtualMem::GetPageSize()
			)
		),
		permAllocator_(permHeap_.start(), permHeap_.end(),
			PoolArena::getMemoryRequirement(sizeof(SourceFile)),
			PoolArena::getMemoryAlignmentRequirement(X_ALIGN_OF(ShaderPermatation)),
			PoolArena::getMemoryOffsetRequirement()
		),
		permArena_(&permAllocator_, "PermPool")
	{
		arena->addChildArena(&permArena_);
	}

	XShaderManager::~XShaderManager()
	{

	}

	void XShaderManager::registerVars(void)
	{
		vars_.RegisterVars();


	}

	void XShaderManager::registerCmds(void)
	{


		ADD_COMMAND_MEMBER("ShaderListHw", this, XShaderManager, &XShaderManager::Cmd_ListHWShaders, core::VarFlag::SYSTEM,
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("shaderListsourcebin", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

		// alternate names
		ADD_COMMAND_MEMBER("listHWShaders", this, XShaderManager, &XShaderManager::Cmd_ListHWShaders, core::VarFlag::SYSTEM,
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("listShaderSource", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

	}

	bool XShaderManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);
		X_LOG1("ShadersManager", "Starting");
		X_PROFILE_NO_HISTORY_BEGIN("ShaderMan", core::profiler::SubSys::RENDER);

		// hotreload support.
		gEnv->pHotReload->addfileType(this, SOURCE_FILE_EXTENSION);
		gEnv->pHotReload->addfileType(this, SOURCE_INCLUDE_FILE_EXTENSION);
		gEnv->pHotReload->addfileType(this, COMPILED_SHADER_FILE_EXTENSION);


		return true;
	}

	bool XShaderManager::shutDown(void)
	{
		X_LOG1("ShadersManager", "Shutting Down");
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);

		// remove the hotreloads.
		gEnv->pHotReload->addfileType(nullptr, SOURCE_FILE_EXTENSION);
		gEnv->pHotReload->addfileType(nullptr, SOURCE_INCLUDE_FILE_EXTENSION);
		gEnv->pHotReload->addfileType(nullptr, COMPILED_SHADER_FILE_EXTENSION);

		freeSourcebin();
		freeHwShaders();

		return true;
	}

	XHWShader* XShaderManager::createHWShader(shader::ShaderType::Enum type, const core::string& entry, const core::string& customDefines,
		shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags, render::shader::VertexFormat::Enum vertFmt)
	{
		ILFlags ilflags = Util::IlFlagsForVertexFormat(vertFmt);

		return hwForName(type, entry, customDefines, static_cast<SourceFile*>(pSourceFile), permFlags, ilflags);
	}

	XHWShader* XShaderManager::createHWShader(shader::ShaderType::Enum type, const core::string& entry,
		const core::string& customDefines, shader::IShaderSource* pSourceFile,
		shader::PermatationFlags permFlags, ILFlags ILFlags)
	{
		return hwForName(type, entry, customDefines, static_cast<SourceFile*>(pSourceFile), permFlags, ILFlags);
	}

	void XShaderManager::releaseHWShader(XHWShader* pHWSHader)
	{
		HWShaderResource* pHWRes = static_cast<HWShaderResource*>(pHWSHader);

		if (pHWRes->removeReference() == 0)
		{
			hwShaders_.releaseAsset(pHWRes);
		}
	}

	bool XShaderManager::compileShader(XHWShader* pHWShader, CompileFlags flags)
	{
		const auto status = pHWShader->getStatus();
		if (status == ShaderStatus::Ready) {
			return true;
		}

		if (status == ShaderStatus::FailedToCompile) {
			X_ERROR("ShadersManager", "can't compile shader, it previously failed to compile");
			return false;
		}

		if (pHWShader->getLock().TryEnter())
		{
			// we adopt the lock we have from tryEnter this is not a re-lock.
			core::UniqueLock<XHWShader::LockType> lock(pHWShader->getLock(), core::adopt_lock);

			// try load it from cache.
			if (vars_.useCache() && shaderBin_.loadShader(pHWShader)) {
				X_ASSERT(pHWShader->getStatus() == ShaderStatus::Ready, "Sahder from cache is not read to rock")();
				return true;
			}

			core::Array<uint8_t> source(arena_);
			if (!sourceBin_.getMergedSource(pHWShader->getShaderSource(), source)) {
				X_ERROR("ShadersManager", "Failed to get source for compiling: \"%s\"", pHWShader->getName().c_str());
				return false;
			}

			if (!pHWShader->compile(source, flags))
			{
				auto errLine = pHWShader->getErrorLineNumber();
				if (errLine >= 0)
				{
					auto info = sourceBin_.getSourceInfoForMergedLine(pHWShader->getShaderSource(), errLine);

					X_ERROR("ShadersManager", "Failed to compile shader. Error in \"%s\" line: %" PRIi32,
						info.name.c_str(), info.line);
				}
				else
				{
					X_ERROR("ShadersManager", "Failed to compile shader");
				}

				return false;
			}

			if (vars_.writeMergedSource())
			{
				X_ASSERT(arena_->isThreadSafe(), "Arena must be thread safe, to dispatch background write")();

				// just dispatch a async write request.
				// the source memory will get cleaned up for us once complete.
				core::IoRequestOpenWrite req(std::move(source));
				getShaderCompileSrc(pHWShader, req.path);
				req.callback.Bind<XShaderManager, &XShaderManager::IoCallback>(this);
				
				gEnv->pFileSys->AddIoRequestToQue(req);
			}

			// save it 
			if (vars_.writeCompiledShaders())
			{
				if (!shaderBin_.saveShader(pHWShader)) {
					X_WARNING("ShadersManager", "Failed to save shader to bin: \"%s\"", pHWShader->getName().c_str());
				}
			}
		}
		else
		{
			// another thread is compiling the shader.
			// we wait for it, maybe run some jobs while we wait?
			int32_t backoff = 0;
			while (1)
			{
				const auto status = pHWShader->getStatus();
				if (status == ShaderStatus::Ready)
				{
					break;
				}
				else if (status == ShaderStatus::FailedToCompile)
				{
					X_ERROR("ShadersManager", "Failed to compile shader");
					return false;
				}
				else
				{
					core::Thread::BackOff(backoff++);
				}
			}
		}

		return true;
	}

	void XShaderManager::compileShader_job(CompileJobInfo* pJobInfo, uint32_t num)
	{
		for (uint32_t i = 0; i < num; i++)
		{
			auto& info = pJobInfo[i];
			info.result = compileShader(info.pHWShader, info.flags);
		}
	}

	shader::IShaderPermatation* XShaderManager::createPermatation(const shader::ShaderStagesArr& stagesIn)
	{
		// ok so for now when we create a permatation we also require all the shaders to be compiled.
		// we return null if a shader fails to compile.

		static_assert(decltype(permArena_)::IS_THREAD_SAFE, "PermArena must be thread safe");
		core::UniquePointer<ShaderPermatation> pPerm = core::makeUnique<ShaderPermatation>(&permArena_, stagesIn, arena_);

		if (!compilePermatation_Int(pPerm.get()))
		{
			return nullptr;
		}

		// we still need to make cb links and get ilFmt even if all the hardware shaders are compiled.
		pPerm->generateMeta();
		
		return pPerm.release();
	}
	
	bool XShaderManager::compilePermatation(shader::IShaderPermatation* pIPerm)
	{
		ShaderPermatation* pPerm = static_cast<ShaderPermatation*>(pIPerm);

		if (pPerm->isCompiled()) {
			return true;
		}

		if (!compilePermatation_Int(pPerm)) {
			return false;
		}

		pPerm->generateMeta();
		return true;
	}
		
	bool XShaderManager::compilePermatation_Int(ShaderPermatation* pPerm)
	{
		CompileFlags flags;

#if X_DEBUG
		flags = CompileFlags::OptimizationLvl0 | CompileFlags::Debug;
#else
		flags = CompileFlags::OptimizationLvl2;
#endif // !X_DEBUG

		// we want to compile this then work out the cbuffer links.
		const auto& stages = pPerm->getStages();
		core::FixedArray<CompileJobInfo, ShaderStage::FLAGS_COUNT> jobInfo;

		// dispatch jobs, to compile all da stages yo.
		for (auto* pHWShader : stages)
		{
			if (!pHWShader || pHWShader->isValid()) {
				continue;
			}

			jobInfo.emplace_back(pHWShader, flags);
		}

		core::Delegate<void(CompileJobInfo*, uint32_t)> del;
		del.Bind<XShaderManager, &XShaderManager::compileShader_job>(this);

		auto* pJob = gEnv->pJobSys->parallel_for_member<XShaderManager>(
			del,
			jobInfo.data(),
			static_cast<uint32_t>(jobInfo.size()),
			core::V2::CountSplitter(1)
			JOB_SYS_SUB_ARG(core::profiler::SubSys::RENDER)
		);

		gEnv->pJobSys->Run(pJob);
		gEnv->pJobSys->Wait(pJob);

		for (const auto& info : jobInfo)
		{
			if (!info.result)
			{
				X_ERROR("ShadersManager", "Failed to compile shader for permatation");
				return false;
			}
		}

		return true;
	}

	void XShaderManager::releaseShaderPermatation(shader::IShaderPermatation* pIPerm)
	{
		// term the perm!
		ShaderPermatation* pPerm = static_cast<ShaderPermatation*>(pIPerm);

		const auto& stages = pPerm->getStages();
		for (auto* pHWShader : stages)
		{
			if (!pHWShader) {
				continue;
			}

			releaseHWShader(pHWShader);
		}

		X_DELETE(pPerm, &permArena_);
	}

	void XShaderManager::getShaderCompileSrc(XHWShader* pShader, core::Path<char>& srcOut)
	{
		srcOut.clear();
		srcOut.appendFmt("shaders/temp/%s.fxcb.%s", pShader->getName().c_str(), SOURCE_FILE_EXTENSION);

		// make sure the directory is created.
		gEnv->pFileSys->createDirectoryTree(srcOut.c_str());
	}

	IShaderSource* XShaderManager::sourceforName(const core::string& name)
	{
		return sourceBin_.loadRawSourceFile(name, false);
	}


	XHWShader* XShaderManager::hwForName(ShaderType::Enum type,
		const core::string& entry, const core::string& customDefines, SourceFile* pSourceFile,
		const shader::PermatationFlags permFlags, ILFlags ILFlags)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		core::StackString512 name;

		const char* pEntry = entry.c_str();
		if (entry.isEmpty())
		{
			pEntry = DEFAULT_SHADER_ENTRY[type];
		}

		core::Hash::SHA1 sha1;
		core::Hash::SHA1Digest::String sha1Buf;
		sha1.update(pEntry);
		sha1.update(customDefines.begin(), customDefines.length());
		sha1.update(permFlags);
		sha1.update(ILFlags);
		sha1.update(type); // include this?
		auto digest = sha1.finalize();

		name.appendFmt("%s@", pSourceFile->getName().c_str());
		name.append(digest.ToString(sha1Buf));


#if X_DEBUG
		X_LOG1("Shader", "HWS for name: \"%s\"", name.c_str());
#endif // !X_DEBUG

		core::string nameStr(name.begin(), name.end());

		// we must have a single lock during the find and create otherwise we have a race.
		core::ScopedLock<HWShaderContainer::ThreadPolicy> lock(hwShaders_.getThreadPolicy());

		HWShaderResource* pHWShaderRes = hwShaders_.findAsset(nameStr);
		if (pHWShaderRes)
		{
			pHWShaderRes->addReference();
			return pHWShaderRes;
		}

		pHWShaderRes = hwShaders_.createAsset(
			nameStr, 
			vars_,
			type,
			nameStr, 
			entry, 
			customDefines,
			pSourceFile,
			permFlags,
			ILFlags,
			arena_
		);

		return pHWShaderRes;
	}

	void XShaderManager::freeSourcebin(void)
	{
		sourceBin_.free();
	}

	void XShaderManager::freeHwShaders(void)
	{
		hwShaders_.free();
	}

	void XShaderManager::listHWShaders(const char* pSearchPattern)
	{
		core::ScopedLock<HWShaderContainer::ThreadPolicy> lock(hwShaders_.getThreadPolicy());

		core::Array<HWShaderContainer::Resource*> sorted_shaders(arena_);
		sorted_shaders.reserve(hwShaders_.size());

		for (const auto& shader : hwShaders_)
		{
			if (!pSearchPattern || core::strUtil::WildCompare(pSearchPattern, shader.first))
			{
				sorted_shaders.emplace_back(shader.second);
			}
		}

		std::sort(sorted_shaders.begin(), sorted_shaders.end(), [](HWShaderContainer::Resource* a, HWShaderContainer::Resource* b) {
				const auto& nameA = a->getName();
				const auto& nameB = b->getName();
				return nameA.compareInt(nameB) < 0;
			}
		);

		X_LOG0("Shader", "------------- ^8Shaders(%" PRIuS ")^7 -------------", hwShaders_.size());
		for(const auto& it : sorted_shaders)
		{
			const auto* pShader = it;

			const char* pFmt = "Name: ^2\"%s\"^7 Status: ^2%s^7 Type: ^2%s^7 IL: ^2%s^7 NumInst: ^2%" PRIi32 
				"^7 Refs: ^2%" PRIi32
				"^7 CompCnt: ^2%" PRIi32;

			int32_t compileCount = -1;

#if X_ENABLE_RENDER_SHADER_RELOAD
			compileCount = pShader->getCompileCount();
#endif // !X_ENABLE_RENDER_SHADER_RELOAD

			// little ugly as can't #if def inside macro call.
			X_LOG0("Shader", pFmt,
				pShader->getName().c_str(),
				ShaderStatus::ToString(pShader->getStatus()),
				ShaderType::ToString(pShader->getType()),
				InputLayoutFormat::ToString(pShader->getILFormat()),
				pShader->getNumInstructions(),
				pShader->getRefCount(),
				compileCount
			);

		}
		X_LOG0("Shader", "------------ ^8Shaders End^7 -------------");
	}

	void XShaderManager::listShaderSources(const char* pSearchPatten)
	{
		sourceBin_.listShaderSources(pSearchPatten);
	}

	void XShaderManager::IoCallback(core::IFileSys&, const core::IoRequestBase*, core::XFileAsync*, uint32_t)
	{


	}

	void XShaderManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys);

		const char* pExt = name.extension(false);
		if (pExt)
		{
			// this is just a cache update ignore this.
			if (core::strUtil::IsEqualCaseInsen(pExt, COMPILED_SHADER_FILE_EXTENSION)) {
				return;
			}

			// ignore .fxcb.hlsl which are merged sources saved out for debuggin.
			if (name.findCaseInsen(SOURCE_MERGED_FILE_EXTENSION)) {
				return;
			}

			core::AssetName assetName(name);
			assetName.replaceSeprators();
			assetName.stripAssetFolder(assetDb::AssetType::SHADER);

			core::string nameStr(assetName.begin(), assetName.end());
			
			if (!sourceBin_.sourceForName(nameStr)) {
				X_WARNING("Shader", "Skipping reload of \"%s\" it's not currently used", nameStr.c_str());
				return;
			}

			// force a reload of the source.
			const auto* pSource = sourceBin_.loadRawSourceFile(nameStr, true);
			if (!pSource) {
				return;
			}

			/*
				we can have dozens of hardware shaders that use this source file.
				each hardware shader holds a pointer to it's shader source.
				so we could just iterate all hardware shaders and invalid any using it.

				might want todo something diffrent if end up with loads of shaders
				like storing refrence lists on the shaders.
			*/

			core::ScopedLock<HWShaderContainer::ThreadPolicy> lock(hwShaders_.getThreadPolicy());

			for (const auto& shader : hwShaders_)
			{
				if (shader.second->getShaderSource() == pSource)
				{
					X_LOG0("Shader", "Reloading: %s", shader.first.c_str());
					shader.second->markStale();
				}
			}

		}
	}

	void XShaderManager::Cmd_ListHWShaders(core::IConsoleCmdArgs* pArgs)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (pArgs->GetArgCount() > 1) {
			pSearchPatten = pArgs->GetArg(1);
		}

		listHWShaders(pSearchPatten);
	}

	void XShaderManager::Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (pArgs->GetArgCount() > 1) {
			pSearchPatten = pArgs->GetArg(1);
		}

		listShaderSources(pSearchPatten);
	}



} // namespace shader

X_NAMESPACE_END