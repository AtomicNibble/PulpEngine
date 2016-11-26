#include "stdafx.h"
#include "ShaderManager.h"
#include "ShaderSourceTypes.h"
#include "Shader.h"

#include <Hashing\crc32.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Util\UniquePointer.h>

#include <IConsole.h>
#include <IFileSys.h>

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
		pCrc32_(nullptr),
		shaderBin_(arena),
		sourceBin_(arena),
		hwShaders_(arena, sizeof(HWShaderResource), X_ALIGN_OF(HWShaderResource)),
		shaders_(arena, sizeof(ShaderResource), X_ALIGN_OF(ShaderResource)),
		pDefaultShader_(nullptr),
		pFixedFunction_(nullptr),
		pFont_(nullptr),
		pGui_(nullptr),

		sourcePoolHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				PoolArena::getMemoryRequirement(core::Max(sizeof(SourceFile),sizeof(ShaderSourceFile))) * MAX_SHADER_SOURCE,
				core::VirtualMem::GetPageSize()
			)
		),
		sourcePoolAllocator_(sourcePoolHeap_.start(), sourcePoolHeap_.end(),
			PoolArena::getMemoryRequirement(sizeof(SourceFile)),
			PoolArena::getMemoryAlignmentRequirement(core::Max(X_ALIGN_OF(SourceFile), X_ALIGN_OF(ShaderSourceFile))),
			PoolArena::getMemoryOffsetRequirement()
		),
		sourcePoolArena_(&sourcePoolAllocator_, "ShaderSourcePool")
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


		pCrc32_ = gEnv->pCore->GetCrc32();

		// hotreload support.
		gEnv->pHotReload->addfileType(this, "hlsl");
		gEnv->pHotReload->addfileType(this, "inc");
		gEnv->pHotReload->addfileType(this, "shader");
		gEnv->pHotReload->addfileType(this, "fxcb");

		ADD_COMMAND_MEMBER("shaderList", this, XShaderManager, &XShaderManager::Cmd_ListShaders, core::VarFlag::SYSTEM, 
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("shaderListsourcebin", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

		// alternate names
		ADD_COMMAND_MEMBER("listShaders", this, XShaderManager, &XShaderManager::Cmd_ListShaders, core::VarFlag::SYSTEM,
			"lists the loaded shaders");
		ADD_COMMAND_MEMBER("listShaderSource", this, XShaderManager, &XShaderManager::Cmd_ListShaderSources, core::VarFlag::SYSTEM,
			"lists the loaded shaders sources");

		vars_.RegisterVars();

		if (!loadCoreShaders()) {
			return false;
		}

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
		gEnv->pHotReload->addfileType(nullptr, "shader");
		gEnv->pHotReload->addfileType(nullptr, "fxcb");

		freeCoreShaders();
		freeSourcebin();
		freeDanglingShaders();
		freeSourceHwShaders();

		return true;
	}



	XShader* XShaderManager::forName(const char* pName)
	{
		core::StackString<assetDb::ASSET_NAME_MAX_LENGTH, char> temp(pName);
		temp.toLower();
		return loadShader(temp.c_str());
	}

	void XShaderManager::releaseShader(XShader* pShader)
	{
		ShaderResource* pshaderRes = static_cast<ShaderResource*>(pShader);

		if (pshaderRes->removeReference() == 0)
		{
			shaders_.releaseAsset(pshaderRes);
		}
	}

	bool XShaderManager::sourceToString(const char* pName, core::string& strOut)
	{
		return sourceBin_.getMergedSource(pName, strOut);
	}

	ShaderVars& XShaderManager::getShaderVars(void)
	{
		return vars_;
	}

	ShaderBin& XShaderManager::getBin(void)
	{
		return shaderBin_;
	}


	bool XShaderManager::loadCoreShaders(void)
	{
		// not loaded, just a blank shader instance
		pDefaultShader_ = createShader("default");

		if ((pFixedFunction_ = forName("ffe")) == nullptr) {
			X_ERROR("Shader", "Failed to load ffe shader");
			return false;
		}
		if ((pFont_ = forName("font")) == nullptr) {
			X_ERROR("Shader", "Failed to load font shader");
			return false;
		}
		if ((pGui_ = forName("gui")) == nullptr) {
			X_ERROR("Shader", "Failed to load gui shader");
			return false;
		}

		return true;
	}

	bool XShaderManager::freeCoreShaders(void)
	{
		releaseShader(pDefaultShader_);
		releaseShader(pFixedFunction_);
		releaseShader(pFont_);
		releaseShader(pGui_);
		
		return true;
	}

	bool XShaderManager::freeDanglingShaders(void)
	{
		for (auto& shaderIt : shaders_)
		{
			const auto& name = shaderIt.second->getName();
			X_WARNING("ShadersManager", "shader \"%s\" is dangling", name.c_str());
		}

		shaders_.free();
		return true;
	}


	XShaderManager::ShaderResource* XShaderManager::getLoadedShader(const char* pName)
	{
		core::string name(pName);
		return shaders_.findAsset(name);
	}


	ShaderSourceFile* XShaderManager::loadShaderFile(const char* pName, bool reload)
	{
		X_ASSERT_NOT_NULL(pName);

		SourceFile* pSourceFile;
		ShaderSourceFile* pShaderSource = nullptr;
		core::StackString512 sourceFileName;

		if ((pSourceFile = loadRawSourceFile(pName, reload)) != nullptr)
		{
			core::XLexer lex(pSourceFile->getFileData().begin(), pSourceFile->getFileData().end());
			core::XLexToken token;

			lex.setFlags(core::LexFlag::ALLOWPATHNAMES);

			if (lex.SkipUntilString("shader"))
			{
				if (!lex.ReadToken(token)) {
					X_ERROR("Shader", "unexpected EOF");
					return nullptr;
				}

				if (!token.isEqual("{")) {
					X_ERROR("Shader", "expected { on line: %i", token.GetLine());
				}

				{
					if (!lex.ExpectTokenString("source")) {
						return nullptr;
					}

					// read the source file name.
					if (!lex.ReadToken(token))
					{
						X_ERROR("Shader", "unexpected EOF");
						return nullptr;
					}

					sourceFileName = core::StackString512(token.begin(), token.end());
				}

				// valid?
				if (sourceFileName.isEmpty())
				{
					X_ERROR("Shader", "invalid source name Line: %i", token.GetLine());
					return nullptr;
				}
				else
				{
					sourceFileName.toLower();
				}

				{
					if (!lex.ExpectTokenString("techniques")) {
						return nullptr;
					}

					if (!lex.ExpectTokenString("{")) {
						return nullptr;
					}

					{
						pShaderSource = X_NEW(ShaderSourceFile, &sourcePoolArena_, "ShaderSourceFile")(arena_);

						while (lex.ReadToken(token))
						{
							if (token.isEqual("}")) {
								break;
							}

							if (!token.isEqual("{")) {
								X_ERROR("Shader", "expected { on line: %i", token.GetLine());
								X_DELETE(pShaderSource, &sourcePoolArena_);
								return nullptr;
							}

							// read a technique
							ShaderSourceFileTechnique tech;

							if (!tech.parse(lex))
							{
								X_ERROR("Shader", "failed to parse tech");
								X_DELETE(pShaderSource, &sourcePoolArena_);
								return nullptr;
							}


							pShaderSource->addTech(tech);
						}
					}

					if (!lex.ExpectTokenString("}")) {
						X_ERROR("Shader", "missing closing brace");
						X_DELETE(pShaderSource, &sourcePoolArena_);
						pShaderSource = nullptr;
					}
				}

			}
			else
			{
				X_ERROR("Shader", "missing 'shader' decariation");
			}

		}

		if (pShaderSource)
		{
			pShaderSource->pHlslFile_ = loadRawSourceFile(sourceFileName.c_str());

			if (!pShaderSource->pHlslFile_) {
				X_DELETE(pShaderSource, &sourcePoolArena_);
				return nullptr;
			}

			core::string name(pName);

			// add the refrences.
			for (auto f : pShaderSource->pHlslFile_->getIncludeArr()) {
				f->addRefrence(name);
			}
			pShaderSource->pHlslFile_->addRefrence(name);


			pShaderSource->pFile_ = pSourceFile;
			pShaderSource->name_ = name;
			// don't combine these, I want to check if just the .shader has changed.
			// seprate to the .hlsl source.
			pShaderSource->sourceCrc32_ = pSourceFile->getSourceCrc32();
			pShaderSource->hlslSourceCrc32_ = pCrc32_->Combine(pSourceFile->getSourceCrc32(),
				pShaderSource->pHlslFile_->getSourceCrc32(),
				safe_static_cast<uint32_t, size_t>(pShaderSource->pHlslFile_->getFileData().size()));

		}

		return pShaderSource;
	}

	SourceFile* XShaderManager::loadRawSourceFile(const char* pName, bool reload)
	{
		return sourceBin_.loadRawSourceFile(pName, reload);
	}


	XShaderManager::ShaderResource* XShaderManager::createShader(const char* pName)
	{
		core::string name(pName);

		X_ASSERT(shaders_.findAsset(name) == nullptr, "Creating asset that already exsists")(pName);

		// check if this shader already exsists.
		auto* pShader = shaders_.createAsset(name, arena_);
		pShader->name_ = name;
		return pShader;
	}


	XShader* XShaderManager::loadShader(const char* pName)
	{
		X_ASSERT_NOT_NULL(pName);

		// already loaded?
		auto*  pShader = getLoadedShader(pName);
		if (pShader) {
			pShader->addReference();
			return pShader;
		}

		// load the source file containing the techniques info.
		ShaderSourceFile* pSource = loadShaderFile(pName);

		if (pSource)
		{
			size_t j, numTecs;

			numTecs = pSource->numTechs();

			pShader = createShader(pName);
			pShader->techs_.resize(pSource->numTechs(), XShaderTechnique(arena_));
			pShader->sourceCrc32_ = pSource->sourceCrc32_;
			pShader->hlslSourceCrc32_ = pSource->hlslSourceCrc32_;
			pShader->pHlslFile_ = pSource->pHlslFile_;

			// I might use only the HLSL crc, do .shader changes matter?
			// for now use hlsl + .shader
			// uint32_t crc = source->pHlslFile->sourceCrc32;
			TechFlags techFlags;
			ILFlags ILflags;
			const ILFlags ILflagSrc = pSource->pHlslFile_->getILFlags();

			for (j = 0; j < numTecs; j++)
			{
				XShaderTechnique& tech = pShader->techs_[j];
				ShaderSourceFileTechnique& srcTech = pSource->techniques_[j];
				tech.assignSourceTech(srcTech);

				// for every input layout we compile all the techFlags 
				// plus one without flags passed.
				uint32_t numILFlags = core::bitUtil::CountBits(ILflagSrc.ToInt());
				uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
				uint32_t i, x;
				for (i = 0; i < numILFlags + 1; i++)
				{
					techFlags.Clear();

					for (x = 0; x < numTechFlags + 1; x++)
					{
						XShaderTechniqueHW hwTech(arena_);

						// create the hardware shaders.
						hwTech.pVertexShader = hwForName(ShaderType::Vertex, pName,
							srcTech.getVertexFunc(),
							pSource->pHlslFile_, techFlags, ILflags);

						hwTech.pPixelShader = hwForName(ShaderType::Pixel, pName,
							srcTech.getPixelFunc(),
							pSource->pHlslFile_, techFlags,  ILflags);

						hwTech.techFlags = techFlags;
						hwTech.ILFlags = ILflags;
						hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
						tech.append(hwTech);

						// add tech flag
						AppendFlagTillEqual(tech.techFlags, techFlags);
					}

					// add in the next flag.
					AppendFlagTillEqual(ILflagSrc, ILflags);
				}

			}

			X_DELETE(pSource, &sourcePoolArena_);
		}

		return pShader;
	}


	XShader* XShaderManager::reloadShader(const char* name)
	{
		X_ASSERT_NOT_NULL(name);

		XShader* pShader = nullptr;
		size_t i, x, numTecs;

		// already loaded?
		pShader = getLoadedShader(name);

		if (pShader)
		{
			// reload the shader file.
			ShaderSourceFile * pShaderSource = loadShaderFile(name, true);
			if (pShaderSource)
			{
				core::UniquePointer<ShaderSourceFile> scopedDelete(&sourcePoolArena_, pShaderSource);

				// we don't reload the .shader if the source is the same.
				if (pShader->sourceCrc32_ != pShaderSource->sourceCrc32_)
				{
					X_LOG0("Shader", "reloading shader: %s", name);

					numTecs = pShaderSource->numTechs();

					// might be more techs etc..
					pShader->techs_.resize(numTecs, XShaderTechnique(arena_));
					pShader->sourceCrc32_ = pShaderSource->sourceCrc32_;
					pShader->hlslSourceCrc32_ = pShaderSource->hlslSourceCrc32_;

					for (i = 0; i < numTecs; i++)
					{
						XShaderTechnique& tech = pShader->techs_[i];
						ShaderSourceFileTechnique& srcTech = pShaderSource->techniques_[i];

						tech.hwTechs.clear();
						tech.assignSourceTech(srcTech);

						// tech flags may have changed.
						// IL flags won't have tho.
						TechFlags techFlags;
						ILFlags ILflags;
						const ILFlags ILflagSrc = pShaderSource->pHlslFile_->getILFlags();

						// for every input layout we compile all the techFlags 
						// plus one without flags passed.
						uint32_t numILFlags = core::bitUtil::CountBits(ILflagSrc.ToInt());
						uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
						uint32_t j;
						for (x = 0; x < numILFlags + 1; x++)
						{
							techFlags.Clear();

							for (j = 0; j < numTechFlags + 1; j++)
							{
								XShaderTechniqueHW hwTech(arena_);

								// create the hardware shaders.
								hwTech.pVertexShader = hwForName(ShaderType::Vertex, name, srcTech.getVertexFunc(),
									pShaderSource->pHlslFile_, techFlags, ILflags);

								hwTech.pPixelShader = hwForName(ShaderType::Pixel, name, srcTech.getPixelFunc(), 
									pShaderSource->pHlslFile_, techFlags, ILflags);

								hwTech.techFlags = techFlags;
								hwTech.ILFlags = ILflags;
								hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
								tech.append(hwTech);

								// add tech flag
								AppendFlagTillEqual(tech.techFlags, techFlags);
							}

							// add in the next flag.
							AppendFlagTillEqual(ILflagSrc, ILflags);
						}
					}
				}
				else if (pShader->hlslSourceCrc32_ != pShaderSource->hlslSourceCrc32_)
				{
					X_LOG0("Shader", "reloading shader source: %s",
						pShader->pHlslFile_->getName().c_str());
					if (pShaderSource)
					{
						numTecs = pShader->numTechs();

						// update crc
						pShader->hlslSourceCrc32_ = pShaderSource->hlslSourceCrc32_;

						for (i = 0; i < numTecs; i++)
						{
							XShaderTechnique& tech = pShader->techs_[i];

							for (x = 0; x < tech.hwTechs.size(); x++)
							{
								XShaderTechniqueHW& hwTech = tech.hwTechs[x];

								const auto& vertEntry = hwTech.pVertexShader->getEntryPoint();
								const auto& pixelEntry = hwTech.pPixelShader->getEntryPoint();

								const TechFlags techFlags = hwTech.techFlags;
								const ILFlags ILFlags = hwTech.ILFlags;

								hwTech.pVertexShader = hwForName(ShaderType::Vertex, name, vertEntry,
									pShaderSource->pHlslFile_, techFlags, ILFlags);

								hwTech.pPixelShader = hwForName(ShaderType::Pixel, name, pixelEntry,
									pShaderSource->pHlslFile_, techFlags, ILFlags);

							}
						}
					}
				}
				else
				{
					uint32_t lastCrc32 = pShader->pHlslFile_->getSourceCrc32();

					SourceFile* Hlslsource = loadRawSourceFile(pShader->pHlslFile_->getFileName().c_str(), true);

					if (Hlslsource)
					{
						if (lastCrc32 != Hlslsource->getSourceCrc32())
						{
							X_LOG0("Shader", "reloading shader source: %s", pShader->pHlslFile_->getName().c_str());

							// the shaders source has changed.
							// we end up here typically when a source file included by 
							// this .shader main .hlsl forcing a reload of the .shader
							// so that the main source can be checked.
							// if we are in this scope we need to recompile all the hardware shaders.
							numTecs = pShader->numTechs();

							for (i = 0; i < numTecs; i++)
							{
								XShaderTechnique& tech = pShader->techs_[i];
								for (x = 0; x < tech.hwTechs.size(); x++)
								{
									XShaderTechniqueHW& hwTech = tech.hwTechs[x];

									const auto& vertEntry = hwTech.pVertexShader->getEntryPoint();
									const auto& pixelEntry = hwTech.pPixelShader->getEntryPoint();

									const TechFlags techFlags = hwTech.techFlags;
									const ILFlags ILFlags = hwTech.ILFlags;

									hwTech.pVertexShader = hwForName(ShaderType::Vertex, name, vertEntry,
										pShaderSource->pHlslFile_, techFlags, ILFlags);

									hwTech.pPixelShader = hwForName(ShaderType::Pixel, name, pixelEntry,
										pShaderSource->pHlslFile_, techFlags, ILFlags);
								}

							}
						}
						else
						{
							X_LOG0("Shader", "shader source: %s has not changed, reload skipped",
								pShader->pHlslFile_->getName().c_str());
						}
					}
				}
			}
		}
		else
		{
#if X_DEBUG
			X_LOG0("Shader", "'%s' not currently used skipping reload", name);
#endif // !X_DEBUG
		}

		return pShader;
	}


	XHWShader* XShaderManager::hwForName(ShaderType::Enum type, const char* pShaderName, 
		const core::string& entry, SourceFile* pSourceFile,
		const TechFlags techFlags, ILFlags ILFlags)
	{
		X_ASSERT_NOT_NULL(pShaderName);
		X_ASSERT_NOT_NULL(pSourceFile);

		core::StackString512 name;

		name.appendFmt("%s@%s", pShaderName, entry);

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

			if (pHWShaderRes->invalidateIfChanged(pSourceFile->getSourceCrc32()))
			{
				// we don't need to do anything currently.		
			}

			return pHWShaderRes;
		}

		pHWShaderRes = hwShaders_.createAsset(nameStr, arena_, type,
			name.c_str(), entry, pSourceFile->getName(), pSourceFile->getSourceCrc32(), techFlags);


		return pHWShaderRes;
	}

	bool XShaderManager::freeSourcebin(void)
	{
		sourceBin_.free();
		return true;
	}

	bool XShaderManager::freeSourceHwShaders(void)
	{
		hwShaders_.free();
		return true;
	}


	void XShaderManager::listShaders(const char* pSarchPatten)
	{
		X_UNUSED(pSarchPatten);

		auto it = shaders_.begin();

		X_LOG0("Shader", "------------- ^8Shaders(%" PRIuS ")^7 -------------", shaders_.size());
		for (; it != shaders_.end(); ++it)
		{
			auto pShader = it->second;

			X_LOG0("Shader", "Name: ^2\"%s\"^7 tecs: %" PRIuS " crc: ^10x%08x^7",
				pShader->name_.c_str(),
				pShader->techs_.size(),
				pShader->sourceCrc32_
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

#if 0
		const char* ext;
		if ((ext = core::strUtil::FileExtension(name)) != nullptr)
		{

			// this is just a cache update ignore this.
			if (core::strUtil::IsEqualCaseInsen(ext, "fxcb")) {
				return true;
			}

			// ignore .fxcb.hlsl which are merged sources saved out for debuggin.
			if (core::strUtil::FindCaseInsensitive(name, ".fxcb.hlsl")) {
				return true;
			}

			// is it source?
			bool isSource = true;

			if (core::strUtil::IsEqualCaseInsen(ext, "shader")) {
				isSource = false;
			}


			if (isSource)
			{
				// if source has changed we need to recompile any shader that uses
				// that source file.
				// how to find the shaders that it uses?
				// if i have some sort of reffrence hirarcy
				// i can take any file and know what shaders use it.
				core::Path<char> temp(name);
				temp.toLower(); // all source is lower case

				ShaderSourceMap::const_iterator it = Sourcebin_.find(core::string(temp.fileName()));
				if (it != Sourcebin_.end())
				{
					// reload the source file.
					loadRawSourceFile(temp.fileName(), true);

					SourceFile* src = it->second;
					for (auto refName : src->refrences)
					{
						reloadShader(refName.c_str());
					}
				}
				else
				{
					// log as not found.
					X_WARNING("Shader", "\"%s\" not used, skipping reload", name);
				}
			}
			else
			{
				core::Path<char> temp(name);
				temp.setExtension("");
				temp.toLower();

				reloadShader(temp.fileName());
			}
		}
		return true;
#else
		X_UNUSED(name);
#endif
	}

	void XShaderManager::Cmd_ListShaders(core::IConsoleCmdArgs* pArgs)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (pArgs->GetArgCount() >= 2) {
			pSearchPatten = pArgs->GetArg(1);
		}

		listShaders(pSearchPatten);
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