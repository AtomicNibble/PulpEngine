#include "stdafx.h"
#include "ShaderManager.h"
#include "ShaderSourceTypes.h"
#include "Shader.h"
#include "HWShader.h"
#include "ILTree.h"

#include <Hashing\crc32.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Util\ScopedPointer.h>

#include <IConsole.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{

		struct PreProEntry
		{
			const char* name;
			PreProType::Enum type;
		};

		struct InputLayoutEntry
		{
			const char* name;
			ILFlag::Enum flag;
		};

		PreProEntry g_ProPros[] =
		{
			{ "include", PreProType::Include },
			{ "define", PreProType::Define },
			{ "undef", PreProType::Undef },
			{ "if", PreProType::If },
			{ "ifdef", PreProType::IfDef },
			{ "ifndef", PreProType::IfNDef },
			{ "else", PreProType::Else },
			{ "endif", PreProType::EndIF },
		};

		// must be prefixed with IL_ (Input Layout)
		InputLayoutEntry g_ILFlags[] = {
			{ "Normal", ILFlag::Normal },
			{ "BiNornmal", ILFlag::BiNormal },
			{ "Color", ILFlag::Color },
		};


		static bool ILFlagFromStr(const char* pStr, ILFlags& flagOut)
		{
			const size_t num = sizeof(g_ILFlags) / sizeof(const char*);
			size_t i;
			for (i = 0; i < num; i++)
			{
				if (core::strUtil::IsEqualCaseInsen(pStr, g_ILFlags[i].name))
				{
					flagOut.Set(g_ILFlags[i].flag);
					return true;
				}
			}
			return false;
		}

		static bool PreProFromStr(core::XLexToken& token, PreProType::Enum& typeOut)
		{
			const size_t num = sizeof(g_ProPros) / sizeof(PreProEntry);
			size_t i;
			for (i = 0; i < num; i++)
			{
				if (core::strUtil::IsEqualCaseInsen(token.begin(), token.end(), g_ProPros[i].name))
				{
					typeOut = g_ProPros[i].type;
					return true;
				}
			}
			return false;
		}

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
		sourcebin_(arena, MAX_SHADER_SOURCE),
		shaderBin_(arena),
		hwShaders_(arena, sizeof(HWShaderResource), X_ALIGN_OF(HWShaderResource)),
		shaders_(arena, sizeof(ShaderResource), X_ALIGN_OF(ShaderResource)),
		ilRoot_(arena),
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

		createInputLayoutTree();
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
		SourceFile* pSourceFile = loadRawSourceFile(pName);

		if (pSourceFile)
		{
			for (auto f : pSourceFile->getIncludeArr())
			{
				strOut.append(f->getFileData());
				strOut.append("\r\n");
			}

			strOut.append(pSourceFile->getFileData());

			return true;
		}
		return false;
	}

	ILTreeNode& XShaderManager::getILTree(void)
	{
		return ilRoot_;
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
		X_ASSERT_NOT_NULL(pName);

		// already loded o.o?

		ShaderSourceMap::iterator it = sourcebin_.find(X_CONST_STRING(pName));
		SourceFile* pSourceFile = nullptr;

		if (it != sourcebin_.end())
		{
			pSourceFile = it->second;

			if (!reload) {
				return pSourceFile;
			}
		}

		// fixed relative folder.
		core::Path<char> path("shaders/");
		path.setFileName(pName);
		if (path.extension() == path.begin()) {
			path.setExtension("shader");
		}

		core::XFileScoped file;

		if (file.openFile(path.c_str(), core::fileMode::READ))
		{
			size_t size = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

			// load into a string for now!
			core::string str;
			str.resize(size);

			uint32_t str_len = safe_static_cast<uint32_t, size_t>(size);

			if (file.read((void*)str.data(), str_len) == str_len)
			{
				// tickle my pickle?
				// check the crc.
				uint32_t crc32 = pCrc32_->GetCRC32(str.data());

				if (pSourceFile)
				{
					// if we reloaded the file and crc32 is same
					// don't both reparsing includes
					if (pSourceFile->getSourceCrc32() == crc32) {
						return pSourceFile;
					}

					pSourceFile->setSourceCrc32(crc32);
					pSourceFile->getIncludeArr().clear();
					pSourceFile->setFileData(str);

					// load any files it includes.
					parseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr(), reload);

					return pSourceFile;
				}
				else
				{

					pSourceFile = X_NEW(SourceFile, &sourcePoolArena_, "SourceFile")(arena_);
					pSourceFile->setName(core::string(pName));
					pSourceFile->setFileName(core::string(path.fileName()));
					pSourceFile->setFileData(str);
					pSourceFile->setSourceCrc32(crc32);

					sourcebin_.insert(std::make_pair(pSourceFile->getFileName(), pSourceFile));

					// load any files it includes.
					parseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr());

					return pSourceFile;
				}
			}
		}

		return nullptr;
	}


	void XShaderManager::parseIncludesAndPrePro_r(SourceFile* pSourceFile,
		core::Array<SourceFile*>& includedFiles, bool reload)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		const auto& fileData = pSourceFile->getFileData();

		core::string::const_str pos = nullptr;
		pos = fileData.find("#include");

		if (pos)
		{
			core::XLexer lexer(fileData.begin(), fileData.end());
			core::XLexToken token;
			core::StackString512 fileName;

			lexer.setFlags(core::LexFlag::ALLOWPATHNAMES);

			while (lexer.SkipUntilString("#"))
			{
				fileName.clear();
				PrePro prepro;

				if (lexer.ReadTokenOnLine(token))
				{
					// check if it's a valid prepro type.
					if (PreProFromStr(token, prepro.type))
					{
						if (prepro.type == PreProType::Include)
						{
							const char* pStart = token.begin() - 1;
							if (lexer.ReadTokenOnLine(token))
							{
								// get the file name, then remove it from the buffer
								// to stop Dx compiler trying to include it.
								fileName.set(token.begin(), token.end());
								memset(const_cast<char*>(pStart), ' ', (token.end() - pStart) + 1);
							}

							// you silly hoe!
							if (fileName.isEmpty())
							{
								X_WARNING("Shader", "invalid #include in: \"%s\" line: %i",
									pSourceFile->getName().c_str(), token.GetLine());
								return;
							}

							// all source names tolower for reload reasons.
							fileName.toLower();

							// load it PLZ.
							SourceFile* pChildSourceFile = loadRawSourceFile(fileName.c_str(), reload);
							if (pChildSourceFile)
							{
								// is this file already included in the tree?
								if (std::find(includedFiles.begin(), includedFiles.end(), pChildSourceFile)
									== includedFiles.end())
								{
									// check if for includes.
									parseIncludesAndPrePro_r(pChildSourceFile, includedFiles);

									// add the include files crc to this one.
									// only after parsing for child includes so that
									// they are included.
									pSourceFile->setSourceCrc32(pCrc32_->Combine(pSourceFile->getSourceCrc32(),
										pChildSourceFile->getSourceCrc32(),
										safe_static_cast<uint32_t, size_t>(pChildSourceFile->getFileData().length())));


									includedFiles.append(pChildSourceFile);
								}
								else
								{
									X_ERROR("Shader", "Recursive file #include for: \"%s\" in shader \"%s\" line: %i",
										fileName.c_str(), pSourceFile->getName().c_str(), token.GetLine());
								}
							}
							else
							{
								X_WARNING("Shader", "File not found: \"%s\"", fileName.c_str());
							}


						}
						else
						{
							// which ones do i care about :|
							// ifdef only tbh, for IL
							if (prepro.type == PreProType::IfDef)
							{
								core::StackString512 ifDefValue;
								if (lexer.ReadTokenOnLine(token))
								{
									if (token.length() > 3) // IL_
									{
										ifDefValue.set(token.begin(), token.end());
										ifDefValue.trim(); // remove white spaces
														   // starts with IL_
										if (ifDefValue.findCaseInsen("IL_") == ifDefValue.begin())
										{
											if (!ILFlagFromStr(ifDefValue.begin() + 3, pSourceFile->getILFlags()))
											{
												X_ERROR("Shader", "invalid InputLayout prepro in shader: % value: %s",
													pSourceFile->getName().c_str(), ifDefValue.c_str());
											}
										}
									}
									else
									{
										// dont care about these.
									}
								}
							}
						}
					}
					else
					{
						// just make use of this buffer
						fileName.set(token.begin(), token.end());
						X_ERROR("Shader", "Invalid prepro in shader source: %s", fileName.c_str());
						continue;
					}
				}

			}

		}
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
						XShaderTechniqueHW hwTech;

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
				core::ScopedPointer<ShaderSourceFile> scopedDelete(&sourcePoolArena_, pShaderSource);

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
								XShaderTechniqueHW hwTech;

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

		pHWShaderRes = hwShaders_.createAsset(nameStr, arena_, *this, type,
			name.c_str(), entry, pSourceFile, techFlags);


		return pHWShaderRes;
	}

	bool XShaderManager::freeSourcebin(void)
	{
		auto it = sourcebin_.begin();;
		for (; it != sourcebin_.end(); ++it) {
			X_DELETE(it->second, &sourcePoolArena_);
		}

		sourcebin_.free();
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

	void XShaderManager::listShaderSources(const char* pSarchPatten)
	{
		X_LOG0("Shader", "--------- ^8Shader Sources(%" PRIuS ")^7 ---------", sourcebin_.size());

		auto printfunc = [](const SourceFile* pSource) {
			X_LOG0("Shader", "Name: ^2\"%s\"^7 crc: ^10x%08x^7",
				pSource->getFileName().c_str(),
				pSource->getSourceCrc32());
		};

		auto it = sourcebin_.begin();
		if (!pSarchPatten) {
			for (; it != sourcebin_.end(); ++it) {
				printfunc(it->second);
			}
		}
		else
		{
			const SourceFile* pSource = nullptr;

			for (; it != sourcebin_.end(); ++it) {
				pSource = it->second;
				if (core::strUtil::WildCompare(pSarchPatten, pSource->getName())) {
					printfunc(pSource);
				}
			}
		}

		X_LOG0("Shader", "--------- ^8Shader Sources End^7 ---------");
	}

	void XShaderManager::createInputLayoutTree(void)
	{
		// all the posible node types.
		ILTreeNode blank(arena_);
		ILTreeNode pos(arena_, "POSITION");
		ILTreeNode uv(arena_, "TEXCOORD");
		ILTreeNode col(arena_, "COLOR");
		ILTreeNode nor(arena_, "NORMAL");
		ILTreeNode tan(arena_, "TANGENT");
		ILTreeNode bin(arena_, "BINORMAL");

		// for shader input layouts the format is not given since the shader
		// don't care what the format comes in as.
		// so how can i work out what the formats are since i support identical sematic layouts
		// with diffrent foramts :(
		//
		// maybe i should just have a sematic format, which can be used to tell if the current input
		// layout will work with the shader :)
		//
		//        .
		//        |
		//       P3F_____
		//       / \     \
		//     T2S  T4F  T3F
		//      |    |__
		//     C4B	    |
		//	  __|	   C4B 
		//	 /  |       |
		// N3F N10	   N3F
		//  |    \
		// TB3F  TB10
		//

		ILTreeNode& uvBase = blank.AddChild(pos).AddChild(uv, InputLayoutFormat::POS_UV);
		uvBase.AddChild(col, InputLayoutFormat::POS_UV_COL).
			AddChild(nor, InputLayoutFormat::POS_UV_COL_NORM).
			AddChild(tan, InputLayoutFormat::POS_UV_COL_NORM_TAN).
			AddChild(bin, InputLayoutFormat::POS_UV_COL_NORM_TAN_BI);

		// double text coords.
		uvBase.AddChild(uv).
			AddChild(col).
			AddChild(nor, InputLayoutFormat::POS_UV2_COL_NORM);

		ilRoot_ = blank;
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