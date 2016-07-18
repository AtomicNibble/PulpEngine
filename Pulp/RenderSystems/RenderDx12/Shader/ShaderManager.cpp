#include "stdafx.h"
#include "ShaderManager.h"
#include "ShaderSourceTypes.h"

#include <Hashing\crc32.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <IConsole.h>

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


		static bool ILFlagFromStr(const char* pStr, Flags<ILFlag>& flagOut)
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
		sourcebin_(arena, 128),
		shaders_(arena, 256)
	{

	}

	XShaderManager::~XShaderManager()
	{


	}



	bool XShaderManager::Init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pCore);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);
		X_ASSERT_NOT_NULL(g_rendererArena);


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

		return true;
	}

	bool XShaderManager::Shutdown(void)
	{
		X_LOG0("ShadersManager", "Shutting Down");
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pHotReload);

		// remove the hotreloads.
		gEnv->pHotReload->addfileType(nullptr, "hlsl");
		gEnv->pHotReload->addfileType(nullptr, "inc");
		gEnv->pHotReload->addfileType(nullptr, "shader");
		gEnv->pHotReload->addfileType(nullptr, "fxcb");

		freeSourcebin();

		return true;
	}



	XShader* XShaderManager::forName(const char* pName)
	{
		core::Path<char> temp(pName);
		temp.toLower();
		return loadShader(temp.c_str());
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

	XShader* XShaderManager::getLoadedShader(const char* pName)
	{
		return nullptr;
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
				if (!lex.ReadToken(token))
				{
					X_ERROR("Shader", "unexpected EOF");
					return nullptr;
				}

				if (!token.isEqual("{"))
				{
					X_ERROR("Shader", "expected { on line: %i", token.GetLine());
				}

				{
					if (!lex.ExpectTokenString("source"))
						return nullptr;

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
						pShaderSource = X_NEW_ALIGNED(ShaderSourceFile, g_rendererArena, "ShaderSourceFile", 8)(arena_);

						while (lex.ReadToken(token))
						{
							if (token.isEqual("}")) {
								break;
							}

							if (!token.isEqual("{")) {
								X_ERROR("Shader", "expected { on line: %i", token.GetLine());
								X_DELETE(pShaderSource, g_rendererArena);
								return nullptr;
							}

							// read a technique
							ShaderSourceFile::Technique tech;

							if (!tech.parse(lex))
							{
								X_ERROR("Shader", "failed to parse tech");
								X_DELETE(pShaderSource, g_rendererArena);
								return nullptr;
							}


							pShaderSource->addTech(tech);
						}
					}

					if (!lex.ExpectTokenString("}")) {
						X_ERROR("Shader", "missing closing brace");
						X_DELETE(pShaderSource, g_rendererArena);
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
				X_DELETE(pShaderSource, g_rendererArena);
				return nullptr;
			}

			// add the refrences.
			for (auto f : pShaderSource->pHlslFile_->includedFiles) {
				f->refrences.insert(core::string(name));
			}
			pShaderSource->pHlslFile_->refrences.insert(core::string(name));


			pShaderSource->pFile_ = pSourceFile;
			pShaderSource->name_ = name;
			// don't combine these, I want to check if just the .shader has changed.
			// seprate to the .hlsl source.
			pShaderSource->sourceCrc32_ = pSourceFile->sourceCrc32;
			pShaderSource->hlslSourceCrc32_ = pCrc32_->Combine(pfile->sourceCrc32,
				pShaderSource->pHlslFile_->sourceCrc32,
				safe_static_cast<uint32_t, size_t>(pShaderSource->pHlslFile_->fileData.size()));

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
					ParseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr(), reload);

					return pSourceFile;
				}
				else
				{

					pSourceFile = X_NEW_ALIGNED(SourceFile, g_rendererArena, "SourceFile", 8);
					pSourceFile->setName(core::string(pName));
					pSourceFile->setFileName(core::string(path.fileName()));
					pSourceFile->setFileData(str);
					pSourceFile->setSourceCrc32(crc32);

					sourcebin_.insert(std::make_pair(pSourceFile->getFileName(), pSourceFile));

					// load any files it includes.
					ParseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr());

					return pSourceFile;
				}
			}
		}

		return nullptr;
	}


	void XShaderManager::ParseIncludesAndPrePro_r(SourceFile* pSourceFile,
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
							const char* start = token.begin() - 1;
							if (lexer.ReadTokenOnLine(token))
							{
								// get the file name, then remove it from the buffer
								// to stop Dx compiler trying to include it.
								fileName.set(token.begin(), token.end());
								memset((char*)start, ' ', (token.end() - start) + 1);
							}

							// you silly hoe!
							if (fileName.isEmpty())
							{
								X_WARNING("Shader", "invalid #include in: \"%s\" line: %i",
									pSourceFile->name.c_str(), token.GetLine());
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
									ParseIncludesAndPrePro_r(pChildSourceFile, includedFiles);

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


	XShader* XShaderManager::createShader(const char* pName)
	{
		X_ASSERT_NOT_NULL(pName);

		// check if this shader already exsists.
		XShader* pShader = getLoadedShader(pName);

		if (pShader)
		{
			pShader->addRef();
		}
		else
		{
			pShader = X_NEW_ALIGNED(XShader, g_rendererArena, "Shader", 16);
			pShader->name_ = name;
			shaders_.AddAsset(name, pShader);
		}

		return pShader;
	}


	XShader* XShaderManager::loadShader(const char* pName)
	{
		X_ASSERT_NOT_NULL(pName);

		XShader* pShader = nullptr;

		// already loaded?
		pShader = getLoadedShader(pName);

		if (pShader) {
			return pShader;
		}

		// load the source file containing the techniques info.
		ShaderSourceFile* pSource = loadShaderFile(pName);

		if (pSource)
		{
			size_t j, numTecs;

			numTecs = pSource->numTechs();

			pShader = createShader(pName);
			pShader->techs_.resize(pSource->numTechs());
			pShader->sourceCrc32_ = pSource->sourceCrc32_;
			pShader->hlslSourceCrc32_ = pSource->hlslSourceCrc32_;
			pShader->pHlslFile_ = pSource->pHlslFile_;

			// I might use only the HLSL crc, do .shader changes matter?
			// for now use hlsl + .shader
			// uint32_t crc = source->pHlslFile->sourceCrc32;
			Flags<TechFlag> techFlags;
			Flags<ILFlag> ILFlags;
			Flags<ILFlag> ILFlagSrc = pSource->pHlslFile_->ILFlags;

			for (j = 0; j < numTecs; j++)
			{
				XShaderTechnique& tech = pShader->techs_[j];
				ShaderSourceFile::Technique& srcTech = pSource->techniques_[j];
				tech = srcTech;

				// for every input layout we compile all the techFlags 
				// plus one without flags passed.
				uint32_t numILFlags = core::bitUtil::CountBits(ILFlagSrc.ToInt());
				uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
				uint32_t i, x;
				for (i = 0; i < numILFlags + 1; i++)
				{
					techFlags.Clear();

					for (x = 0; x < numTechFlags + 1; x++)
					{
						XShaderTechniqueHW hwTech;

						// create the hardware shaders.
						hwTech.pVertexShader = XHWShader::forName(name,
							srcTech.vertex_func_,
							pSource->pHlslFile_->fileName.c_str(), techFlags,
							ShaderType::Vertex, ILFlags, source->pHlslFile_->sourceCrc32);

						hwTech.pPixelShader = XHWShader::forName(name,
							srcTech.pixel_func_,
							pSource->pHlslFile_->fileName.c_str(), techFlags,
							ShaderType::Pixel, ILFlags, source->pHlslFile_->sourceCrc32);

						hwTech.techFlags = techFlags;
						hwTech.ILFlags = ILFlags;
						hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
						tech.append(hwTech);

						// add tech flag
						AppendFlagTillEqual(tech.techFlags, techFlags);
					}

					// add in the next flag.
					AppendFlagTillEqual(ILFlagSrc, ILFlags);
				}

				tech.resetCurHWTech();
			}

			X_DELETE(pSource, g_rendererArena);
		}

		return pShader;
	}


	XShader* XShaderManager::reloadShader(const char* name)
	{
		X_ASSERT_NOT_NULL(name);

		XShader* pShader = nullptr;
		ShaderSourceFile* pShaderSource = nullptr;
		size_t i, x, numTecs;

		// already loaded?
		pShader = getLoadedShader(name);

		if (pShader)
		{

			// reload the shader file.
			pShaderSource = loadShaderFile(name, true);
			if (pShaderSource)
			{
				// we don't reload the .shader if the source is the same.
				if (pShader->sourceCrc32_ != pShaderSource->sourceCrc32_)
				{
					X_LOG0("Shader", "reloading shader: %s", name);

					numTecs = pShaderSource->numTechs();

					// might be more techs etc..
					pShader->techs_.resize(numTecs);
					pShader->sourceCrc32_ = pShaderSource->sourceCrc32_;
					pShader->hlslSourceCrc32_ = pShaderSource->hlslSourceCrc32_;

					for (i = 0; i < numTecs; i++)
					{
						XShaderTechnique& tech = pShader->techs_[i];
						ShaderSourceFile::Technique& srcTech = pShaderSource->techniques_[i];

						tech.hwTechs.clear();
						tech = srcTech;
						// tech flags may have changed.
						// IL flags won't have tho.
						Flags<TechFlag> techFlags;
						Flags<ILFlag> ILFlags;
						Flags<ILFlag> ILFlagSrc = pShaderSource->pHlslFile_->ILFlags;

						// for every input layout we compile all the techFlags 
						// plus one without flags passed.
						uint32_t numILFlags = core::bitUtil::CountBits(ILFlagSrc.ToInt());
						uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
						uint32_t j;
						for (x = 0; x < numILFlags + 1; x++)
						{
							techFlags.Clear();

							for (j = 0; j < numTechFlags + 1; j++)
							{
								XShaderTechniqueHW hwTech;

								// create the hardware shaders.
								hwTech.pVertexShader = XHWShader::forName(name,
									srcTech.vertex_func_,
									pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Vertex, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);

								hwTech.pPixelShader = XHWShader::forName(name,
									srcTech.pixel_func_,
									pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Pixel, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);

								hwTech.techFlags = techFlags;
								hwTech.ILFlags = ILFlags;
								hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
								tech.append(hwTech);

								// add tech flag
								AppendFlagTillEqual(tech.techFlags, techFlags);
							}

							// add in the next flag.
							AppendFlagTillEqual(ILFlagSrc, ILFlags);
						}

						tech.resetCurHWTech();

					}
				}
				else if (pShader->hlslSourceCrc32_ != pShaderSource->hlslSourceCrc32_)
				{
					X_LOG0("Shader", "reloading shader source: %s",
						shader->pHlslFile_->name.c_str());
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

								const char* vertEntry = hwTech.pVertexShader->getEntryPoint();
								const char* pixelEntry = hwTech.pPixelShader->getEntryPoint();

								TechFlags techFlags = hwTech.techFlags;
								ILFlags ILFlags = hwTech.ILFlags;


								hwTech.pVertexShader = XHWShader::forName(name, vertEntry,
									pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Vertex, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);

								hwTech.pPixelShader = XHWShader::forName(name, pixelEntry,
									pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Pixel, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);
							}
						}
					}
				}
				else
				{
					uint32_t lastCrc32 = pShader->pHlslFile_->sourceCrc32;

					SourceFile* Hlslsource = loadRawSourceFile(pShader->pHlslFile_->fileName.c_str(), true);

					if (Hlslsource)
					{
						if (lastCrc32 != Hlslsource->sourceCrc32)
						{
							X_LOG0("Shader", "reloading shader source: %s", pShader->pHlslFile_->name.c_str());

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

									const char* vertEntry = hwTech.pVertexShader->getEntryPoint();
									const char* pixelEntry = hwTech.pPixelShader->getEntryPoint();

									TechFlags techFlags = hwTech.techFlags;
									ILFlags ILFlags = hwTech.ILFlags;

									hwTech.pVertexShader = XHWShader::forName(name, vertEntry,
										pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
										ShaderType::Vertex, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);

									hwTech.pPixelShader = XHWShader::forName(name, pixelEntry,
										pShaderSource->pHlslFile_->fileName.c_str(), techFlags,
										ShaderType::Pixel, ILFlags, pShaderSource->pHlslFile_->sourceCrc32);
								}

							}
						}
						else
						{
							X_LOG0("Shader", "shader source: %s has not changed, reload skipped",
								pShader->pHlslFile_->name.c_str());
						}
					}
				}

				X_DELETE(pShaderSource, g_rendererArena);
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




	bool XShaderManager::freeSourcebin(void)
	{
		ShaderSourceMap::iterator it = sourcebin_.begin();;
		for (; it != sourcebin_.end(); ++it) {
			X_DELETE(it->second, g_rendererArena);
		}

		sourcebin_.free();
		return true;
	}

	void XShaderManager::listShaders(void)
	{
#if 0
		render::XRenderResourceContainer::ResourceConstItor it = shaders_.begin();
		XShader* pShader;

		X_LOG0("Shader", "------------- ^8Shaders(%" PRIuS ")^7 -------------", shaders_.size());

		for (; it != shaders_.end(); ++it)
		{
			pShader = static_cast<XShader*>(it->second);

			X_LOG0("Shader", "Name: ^2\"%s\"^7 tecs: %" PRIuS " crc: ^10x%08x^7 vertexFmt: %s",
				pShader->name_.c_str(),
				pShader->techs_.size(),
				pShader->sourceCrc32_,
				VertexFormat::toString(pShader->vertexFmt_));
		}
		X_LOG0("Shader", "------------ ^8Shaders End^7 -------------");
#endif
	}

	void XShaderManager::listShaderSources(void)
	{
		ShaderSourceMap::const_iterator it = sourcebin_.begin();
		const SourceFile* pSource;

		X_LOG0("Shader", "--------- ^8Shader Sources(%" PRIuS ")^7 ---------", sourcebin_.size());

		for (; it != sourcebin_.end(); ++it)
		{
			pSource = it->second;

			X_LOG0("Shader", "Name: ^2\"%s\"^7 crc: ^10x%08x^7",
				pSource->getFileName().c_str(),
				pSource->getSourceCrc32());
		}

		X_LOG0("Shader", "--------- ^8Shader Sources End^7 ---------");
	}


	void XShaderManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys);


	}

	void XShaderManager::writeSourceToFile(core::XFile* pFile, const SourceFile* pSourceFile)
	{
		X_ASSERT_NOT_NULL(pFile);
		X_ASSERT_NOT_NULL(pSourceFile);

		pFile->printf("\n// ======== %s ========\n\n", pSourceFile->getFileName().c_str());
		pFile->write(pSourceFile->getFileData().c_str(), safe_static_cast<uint32_t, size_t>(pSourceFile->getFileData().length()));
	}



	void XShaderManager::Cmd_ListShaders(core::IConsoleCmdArgs* pArgs)
	{
		X_UNUSED(pArgs);

		listShaders();
	}

	void XShaderManager::Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs)
	{
		X_UNUSED(pArgs);

		listShaderSources();
	}



} // namespace shader

X_NAMESPACE_END