#include "stdafx.h"
#include "SourceBin.h"
#include "ShaderSourceTypes.h"

#include <IFileSys.h>

#include <Hashing\crc32.h>
#include <String\XParser.h>
#include <String\StringHash.h>
#include <Memory\VirtualMem.h>

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{

		bool PreProFromStr(core::XLexToken& token, core::PreProType::Enum& typeOut)
		{
			using namespace core::Hash::Fnva1Literals;
			using namespace core;

			// too long?
			const auto len = token.length();
			if (len > 8) {
				return false;
			}

			static_assert(PreProType::ENUM_COUNT == 10, "PreProType count changed? this code needs updating.");
			switch (core::Hash::Fnv1aHash(token.begin(), len))
			{
				case "include"_fnv1a:
					typeOut = PreProType::Include;
					break;
				case "define"_fnv1a:
					typeOut = PreProType::Define;
					break;
				case "undef"_fnv1a:
					typeOut = PreProType::Undef;
					break;
				case "if"_fnv1a:
					typeOut = PreProType::If;
					break;
				case "ifdef"_fnv1a:
					typeOut = PreProType::IfDef;
					break;
				case "ifndef"_fnv1a:
					typeOut = PreProType::IfNDef;
					break;
				case "else"_fnv1a:
					typeOut = PreProType::Else;
					break;
				case "elif"_fnv1a:
					typeOut = PreProType::ElseIf;
					break;
				case "endif"_fnv1a:
					typeOut = PreProType::EndIF;
					break;
				default:
					return false;
			}

			return true;
		}

		bool ILFlagFromStr(const char* pStr, ILFlag::Enum& flagOut)
		{
			using namespace core::Hash::Fnva1Literals;

			core::StackString<128, char> strUpper(pStr);
			strUpper.toUpper();
			
			static_assert(ILFlag::FLAGS_COUNT == 3, "ILFlag count changed? this code needs updating.");
			switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
			{
				case "NORMAL"_fnv1a:
					flagOut = ILFlag::Normal;
				case "BINORMAL"_fnv1a:
					flagOut = ILFlag::BiNormal;
				case "COLOR"_fnv1a:
					flagOut = ILFlag::Color;
				default:
					return false;
			}

			return true;
		}

	} // namespace


	SourceBin::SourceBin(core::MemoryArenaBase* arena) :
		arena_(arena),
		pCrc32_(nullptr),
		source_(arena, MAX_SHADER_SOURCE),
		sourcePoolHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				PoolArena::getMemoryRequirement(core::Max(sizeof(SourceFile), sizeof(ShaderSourceFile))) * MAX_SHADER_SOURCE,
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
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pCore);

		pCrc32_ = gEnv->pCore->GetCrc32();
	}

	void SourceBin::free(void)
	{
		for (auto& s : source_)
		{
			X_DELETE(s.second, &sourcePoolArena_);
		}

		source_.free();
	}

	bool SourceBin::getMergedSource(const char* pName, core::string& strOut)
	{
		SourceFile* pSourceFile = loadRawSourceFile(pName);

		if (pSourceFile)
		{
			size_t size = pSourceFile->getFileData().length();
			for (const auto& f : pSourceFile->getIncludeArr())
			{
				size += f->getFileData().length();
			}

			strOut.reserve(size);

			for (const auto& f : pSourceFile->getIncludeArr())
			{
				strOut.append(f->getFileData());
				strOut.append("\r\n");
			}

			strOut.append(pSourceFile->getFileData());

			return true;
		}
		return false;
	}


	SourceFile* SourceBin::loadRawSourceFile(const char* pName, bool reload)
	{
		SourceFile* pSourceFile = sourceForName(pName);

		if (pSourceFile && !reload) {
			return pSourceFile;
		}

		core::Path<char> path("shaders/");
		path.setFileName(pName);
		if (path.extension() == path.begin()) {
			path.setExtension("shader");
		}

		core::XFileScoped file;
		if (!file.openFile(path.c_str(), core::fileMode::READ)) {

			return nullptr;
		}

		size_t size = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

		// load into a string for now!
		core::string str;
		str.resize(size);

		uint32_t str_len = safe_static_cast<uint32_t>(size);

		if (file.read((void*)str.data(), str_len) != str_len) {

			return nullptr;
		}

		// tickle my pickle?
		// check the crc.
		uint32_t crc32 = pCrc32_->GetCRC32(str.data(), str.length());

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


		pSourceFile = X_NEW(SourceFile, &sourcePoolArena_, "SourceFile")(arena_);
		pSourceFile->setName(core::string(pName));
		pSourceFile->setFileName(core::string(path.fileName()));
		pSourceFile->setFileData(str);
		pSourceFile->setSourceCrc32(crc32);

		source_.insert(std::make_pair(pSourceFile->getFileName(), pSourceFile));

		// load any files it includes.
		parseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr());

		return pSourceFile;
	}

	SourceFile* SourceBin::sourceForName(const char* pName)
	{
		auto it = source_.find(X_CONST_STRING(pName));
		if (it != source_.end()) {
			return it->second;
		}

		return nullptr;
	}

	void SourceBin::parseIncludesAndPrePro_r(SourceFile* pSourceFile, SourceRefArr& includedFiles, bool reload)
	{
		const auto& fileData = pSourceFile->getFileData();

		// any includes to process?
		auto pos = fileData.find("#include");
		if (!pos) {
			return;
		}

		core::XLexer lexer(fileData.begin(), fileData.end());
		core::XLexToken token;
		core::StackString512 fileName;

		lexer.setFlags(core::LexFlag::ALLOWPATHNAMES);

		while (lexer.SkipUntilString("#"))
		{
			fileName.clear();
			core::PreProType::Enum preproType;

			if (lexer.ReadTokenOnLine(token))
			{
				// check if it's a valid prepro type.
				if (PreProFromStr(token, preproType))
				{
					if (preproType == core::PreProType::Include)
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
						if (preproType == core::PreProType::IfDef)
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
										ILFlag::Enum ilFlag;
										if (!ILFlagFromStr(ifDefValue.begin() + 3, ilFlag))
										{
											X_ERROR("Shader", "invalid InputLayout prepro in shader: % value: %s",
												pSourceFile->getName().c_str(), ifDefValue.c_str());
										}
										else
										{
											// add the flag.
											pSourceFile->setILFlags(pSourceFile->getILFlags() | ilFlag);
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
					X_ERROR("Shader", "Invalid prepro in shader source: %.*s", token.length(), token.begin());
					continue;
				}
			}
		}
	}

	void SourceBin::listShaderSources(const char* pSearchPatten)
	{
		X_LOG0("Shader", "--------- ^8Shader Sources(%" PRIuS ")^7 ---------", source_.size());

		auto printfunc = [](const SourceFile* pSource) {
			X_LOG0("Shader", "Name: ^2\"%s\"^7 crc: ^10x%08x^7",
				pSource->getFileName().c_str(),
				pSource->getSourceCrc32());
		};

		if (!pSearchPatten) {
			for(const auto& s : source_) {
				printfunc(s.second);
			}
		}
		else
		{
			const SourceFile* pSource = nullptr;

			for (const auto& s : source_) {
				pSource = s.second;
				if (core::strUtil::WildCompare(pSearchPatten, pSource->getName())) {
					printfunc(pSource);
				}
			}
		}

		X_LOG0("Shader", "--------- ^8Shader Sources End^7 ---------");
	}

} // namespace shader

X_NAMESPACE_END

