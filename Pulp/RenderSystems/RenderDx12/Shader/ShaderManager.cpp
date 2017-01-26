#include "stdafx.h"
#include "ShaderManager.h"
#include "ShaderPermatation.h"

#include <Hashing\crc32.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Util\UniquePointer.h>
#include <Threading\JobSystem2.h>

#include <IConsole.h>
#include <IFileSys.h>

#include <../../tools/ShaderLib/ShaderSourceTypes.h>

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
				for (size_t i = 0; i < 32; i++)
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
		hwShaders_(arena, sizeof(HWShaderResource), X_ALIGN_OF(HWShaderResource))
	{

	}

	XShaderManager::~XShaderManager()
	{

	}



	bool XShaderManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pCore);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);
		X_ASSERT_NOT_NULL(g_rendererArena);
		X_LOG1("ShadersManager", "Starting");

		// hotreload support.
		gEnv->pHotReload->addfileType(this, "hlsl");
		gEnv->pHotReload->addfileType(this, "inc");
		gEnv->pHotReload->addfileType(this, "fxcb");

		ADD_COMMAND_MEMBER("ShaderListHw", this, XShaderManager, &XShaderManager::Cmd_ListHWShaders, core::VarFlag::SYSTEM,
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("shaderListsourcebin", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

		// alternate names
		ADD_COMMAND_MEMBER("listHWShaders", this, XShaderManager, &XShaderManager::Cmd_ListHWShaders, core::VarFlag::SYSTEM,
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("listShaderSource", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

		vars_.RegisterVars();

		return true;
	}

	bool XShaderManager::shutDown(void)
	{
		X_LOG1("ShadersManager", "Shutting Down");
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);

		// remove the hotreloads.
		gEnv->pHotReload->addfileType(nullptr, "hlsl");
		gEnv->pHotReload->addfileType(nullptr, "inc");
		gEnv->pHotReload->addfileType(nullptr, "fxcb");

		freeSourcebin();
		freeHwShaders();

		return true;
	}

	IHWShader* XShaderManager::createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile)
	{
		XHWShader* pHW = hwForName(type, entry, static_cast<SourceFile*>(pSourceFile), 0, 0);

		return pHW;
	}

	void XShaderManager::releaseHWShader(IHWShader* pHWSHader)
	{
		releaseHWShader(static_cast<XHWShader*>(pHWSHader));
	}

	shader::IShaderPermatation* XShaderManager::createPermatation(const shader::ShaderStagesArr& stages)
	{
		// ok so for now when we create a permatation we also require all the shaders to be compiled.
		// we return null if a shader fails to compile.
		core::UniquePointer<ShaderPermatation> pPerm = core::makeUnique<ShaderPermatation>(arena_, stages, arena_);

		if (!pPerm->isCompiled())
		{
			// we want to compile this then work out the cbuffer links.
			const auto& stages = pPerm->getStages();
			for (auto* pHWShader : stages)
			{
				if (!pHWShader) {
					continue;
				}

				auto status = pHWShader->getStatus();
				
				if (status == ShaderStatus::ReadyToRock) {
					continue;
				}
				if (status == ShaderStatus::FailedToCompile) {
					X_ERROR("ShadersManager", "can't create permatation, a hw shader failed to compile");
					return false;
				}

				// try load it from cache.
				if (shaderBin_.loadShader(pHWShader)) {
					X_ASSERT(pHWShader->getStatus() == ShaderStatus::ReadyToRock, "Sahder from cache is not read to rock")();
					continue;
				}

				// compile from source.

				core::string source;
				if (!sourceBin_.getMergedSource(pHWShader->getSourceFileName(), source)) {
					X_ERROR("ShadersManager", "Failed to get source for compiling: \"%s\"", pHWShader->getSourceFileName().c_str());
					return false;
				}

				if (vars_.writeMergedSource())
				{
					core::Path<char> src;
					getShaderCompileSrc(pHWShader, src);

					core::XFileScoped fileOut;
					if (fileOut.openFile(src.c_str(), core::fileModeFlags::RECREATE | core::fileModeFlags::WRITE | core::fileModeFlags::SHARE))
					{
						fileOut.write(source.data(), source.length());
					}
				}

				if (!pHWShader->compile(source))
				{
					X_ERROR("ShadersManager", "Failed to compile shader for permatation");
					return false;
				}

				// save it 
				if (vars_.writeCompiledShaders())
				{
					if (!shaderBin_.saveShader(pHWShader)) {
						X_WARNING("ShadersManager", "Failed to save shader to bin: \"%s\"", pHWShader->getName().c_str());
					}
				}
			}

		}

		// we still need to make cb links and get ilFmt even if all the hardware shaders are compiled.
		pPerm->generateMeta();
		
		return pPerm.release();
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

		X_DELETE(pPerm, arena_);
	}

	void XShaderManager::getShaderCompileSrc(XHWShader* pShader, core::Path<char>& srcOut)
	{
		srcOut.clear();
		srcOut.appendFmt("shaders/temp/%s.fxcb.hlsl", pShader->getName().c_str());

		// make sure the directory is created.
		gEnv->pFileSys->createDirectoryTree(srcOut.c_str());
	}

	IShaderSource* XShaderManager::sourceforName(const char* pName)
	{
		return sourceBin_.loadRawSourceFile(pName, false);
	}


	ShaderVars& XShaderManager::getShaderVars(void)
	{
		return vars_;
	}

	ShaderBin& XShaderManager::getBin(void)
	{
		return shaderBin_;
	}


	SourceFile* XShaderManager::loadRawSourceFile(const char* pName, bool reload)
	{
		return sourceBin_.loadRawSourceFile(pName, reload);
	}


	XHWShader* XShaderManager::hwForName(ShaderType::Enum type,
		const core::string& entry, SourceFile* pSourceFile,
		const TechFlags techFlags, ILFlags ILFlags)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		core::StackString512 name;

		name.appendFmt("%s@%s", pSourceFile->getName().c_str(), entry);

		// macros are now part of the name.
		name.appendFmt("_%x", techFlags.ToInt());

		// input layout flags are also part of the name.
		name.appendFmt("_%x", ILFlags.ToInt());


#if X_DEBUG
		X_LOG1("Shader", "HWS for name: \"%s\"", name.c_str());
#endif // !X_DEBUG

		core::string nameStr(name.c_str());

		HWShaderResource* pHWShaderRes = hwShaders_.findAsset(nameStr);
		if (pHWShaderRes)
		{
			pHWShaderRes->addReference();

#if 0
			if (pHWShaderRes->invalidateIfChanged(pSourceFile->getSourceCrc32()))
			{
				// we don't need to do anything currently.		
			}
#endif

			return pHWShaderRes;
		}

		pHWShaderRes = hwShaders_.createAsset(nameStr, arena_, type,
			name.c_str(), entry, pSourceFile->getName(), pSourceFile->getSourceCrc32(), techFlags);


		return pHWShaderRes;
	}

	void XShaderManager::releaseHWShader(XHWShader* pHWSHader)
	{
		HWShaderResource* pHWRes = static_cast<HWShaderResource*>(pHWSHader);

		if (pHWRes->removeReference() == 0)
		{
			hwShaders_.releaseAsset(pHWRes);
		}
	}

	bool XShaderManager::freeSourcebin(void)
	{
		sourceBin_.free();
		return true;
	}

	bool XShaderManager::freeHwShaders(void)
	{
		hwShaders_.free();
		return true;
	}

	void XShaderManager::listHWShaders(const char* pSarchPatten)
	{
		X_UNUSED(pSarchPatten);


		X_LOG0("Shader", "------------- ^8Shaders(%" PRIuS ")^7 -------------", hwShaders_.size());
		for(const auto& it : hwShaders_)
		{
			const auto* pShader = it.second;

			X_LOG0("Shader", "Name: ^2\"%s\"",
				pShader->getName().c_str()
			);
		}
		X_LOG0("Shader", "------------ ^8Shaders End^7 -------------");
	}

	void XShaderManager::listShaderSources(const char* pSearchPatten)
	{
		sourceBin_.listShaderSources(pSearchPatten);
	}

	void XShaderManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys);

		const char* pExt = name.extension();
		if (pExt)
		{
			// this is just a cache update ignore this.
			if (core::strUtil::IsEqualCaseInsen(pExt, "fxcb")) {
				return;
			}

			// ignore .fxcb.hlsl which are merged sources saved out for debuggin.
			if (name.findCaseInsen(".fxcb.hlsl")) {
				return;
			}

			// it's a source file change.


		}
	}

	void XShaderManager::Cmd_ListHWShaders(core::IConsoleCmdArgs* pArgs)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (pArgs->GetArgCount() >= 2) {
			pSearchPatten = pArgs->GetArg(1);
		}

		listHWShaders(pSearchPatten);
	}

	void XShaderManager::Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (pArgs->GetArgCount() >= 2) {
			pSearchPatten = pArgs->GetArg(1);
		}

		listShaderSources(pSearchPatten);
	}



} // namespace shader

X_NAMESPACE_END