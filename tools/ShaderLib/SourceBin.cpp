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
			using namespace core::Hash::Fnv1Literals;
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
			using namespace core::Hash::Fnv1Literals;

			core::StackString<128, char> strUpper(pStr);
			strUpper.toUpper();
			
			static_assert(ILFlag::FLAGS_COUNT == 4, "ILFlag count changed? this code needs updating.");
			switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
			{
				case "UV2"_fnv1a:
					flagOut = ILFlag::Uv2;
					break;
				case "NORMAL"_fnv1a:
					flagOut = ILFlag::Normal;
					break;
				case "BINORMAL"_fnv1a:
					flagOut = ILFlag::BiNormal;
					break;
				case "COLOR"_fnv1a:
					flagOut = ILFlag::Color;
					break;
				default:
					return false;
			}

			return true;
		}

		SourceFile* const INVALID_SOURCE = reinterpret_cast<SourceFile*>(std::numeric_limits<uintptr_t>::max());
		


	} // namespace


	SourceBin::SourceBin(core::MemoryArenaBase* arena) :
		arena_(arena),
		pCrc32_(nullptr),
		source_(arena, MAX_SHADER_SOURCE),
		sourcePoolHeap_(
			core::bitUtil::RoundUpToMultiple<size_t>(
				PoolArena::getMemoryRequirement(sizeof(SourceFile)) * MAX_SHADER_SOURCE,
				core::VirtualMem::GetPageSize()
			)
		),
		sourcePoolAllocator_(sourcePoolHeap_.start(), sourcePoolHeap_.end(),
			PoolArena::getMemoryRequirement(sizeof(SourceFile)),
			PoolArena::getMemoryAlignmentRequirement(X_ALIGN_OF(SourceFile)),
			PoolArena::getMemoryOffsetRequirement()
		),
		sourcePoolArena_(&sourcePoolAllocator_, "ShaderSourcePool")
	{
		arena->addChildArena(&sourcePoolArena_);

		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pCore);

		pCrc32_ = gEnv->pCore->GetCrc32();
	}

	void SourceBin::free(void)
	{
		core::CriticalSection::ScopedLock lock(cs_);

		for (auto& s : source_)
		{
			X_DELETE(s.second, &sourcePoolArena_);
		}

		source_.free();
	}

	bool SourceBin::getMergedSource(const SourceFile* pSourceFile, ByteArr& merged)
	{
		if (pSourceFile)
		{
			// we take read locks of everything while building merged source.
			core::ScopedLockShared<SourceFile::LockType> lock(pSourceFile->lock);

			size_t size = pSourceFile->getFileData().size();
			for (const auto* pSf : pSourceFile->getIncludeArr())
			{
				pSf->lock.EnterShared();
				size += pSf->getFileData().size();
			}

			merged.reserve(size);

			for (const auto* pSf : pSourceFile->getIncludeArr())
			{
				merged.append(pSf->getFileData());
				merged.append('\n');

				pSf->lock.LeaveShared();
			}

			merged.append(pSourceFile->getFileData());

			if (merged.isEmpty()) {
				X_BREAKPOINT;
				return false;
			}

			return true;
		}

		return false;
	}

	SourceBin::SourceInfo SourceBin::getSourceInfoForMergedLine(const SourceFile* pSourceFile, size_t line)
	{
		X_ASSERT_NOT_NULL(pSourceFile);

		SourceInfo info;
		size_t currentLine = 0;

		core::ScopedLockShared<SourceFile::LockType> lock(pSourceFile->lock);

		for (const auto* pSf : pSourceFile->getIncludeArr())
		{
			core::ScopedLockShared<SourceFile::LockType> lock_inc(pSf->lock);

			const auto& data = pSf->getFileData();

			auto lines = core::strUtil::NumLines(
				reinterpret_cast<const char*>(data.begin()),
				reinterpret_cast<const char*>(data.end())
			);

			if (line < (currentLine + lines))
			{
				info.line = safe_static_cast<int32_t>(line - currentLine);
				info.name = pSourceFile->getName();
				return info;
			}

			currentLine += lines;
		}

		// assume it's in this file or out of range..
		info.line = safe_static_cast<int32_t>(line - currentLine);
		info.name = pSourceFile->getName();
		return info;
	}


	SourceFile* SourceBin::loadRawSourceFile(const core::string& name, bool reload)
	{
		SourceFile** pSourceFileRef = nullptr;
		bool loaded = true;

		core::string namelc(name);
		namelc.toLower();

		{
			core::CriticalSection::ScopedLock lock(cs_);

			auto it = source_.find(namelc);
			if (it != source_.end())
			{
				pSourceFileRef = &it->second;
			}
			else
			{
				loaded = false;
				auto insertIt = source_.insert(std::make_pair(namelc, nullptr));
				pSourceFileRef = &insertIt.first->second;
			}
		}

		// if already loaded, make sure it's finished loaded.
		if (loaded)
		{
			int32_t backOff = 0;

			// wait for it to finish loading on another thread.
			while (*pSourceFileRef == nullptr)
			{
				core::Thread::BackOff(backOff++);
			}

			if (*pSourceFileRef == INVALID_SOURCE) {
				return nullptr;
			}

			if (!reload) {
				return *pSourceFileRef;
			}
		}

		// we must load it.
		core::Path<char> path("shaders/");
		path.setFileName(namelc.begin(), namelc.end());

		core::XFileScoped file;
		if (!file.openFile(path.c_str(), core::fileMode::READ | core::fileMode::SHARE)) {
			X_WARNING("Shader", "File not found: \"%s\"", namelc.c_str());
			*pSourceFileRef = INVALID_SOURCE;
			return nullptr;
		}

		size_t size = safe_static_cast<size_t>(file.remainingBytes());
		ByteArr data(arena_, size);

		if (file.read(data.data(), data.size()) != data.size()) {
			X_WARNING("Shader", "Failed to read all of file: \"%s\"", namelc.c_str());
			*pSourceFileRef = INVALID_SOURCE;
			return nullptr;
		}

		uint32_t crc32 = pCrc32_->GetCRC32(data.data(), data.size());

		auto process = [&](SourceFile* pSourceFile) {
			pSourceFile->setFileData(std::move(data), crc32);

			// load any files it includes.
			parseIncludesAndPrePro_r(pSourceFile, pSourceFile->getIncludeArr(), reload);

			pSourceFile->applyRefrences();
		};

		if (reload)
		{
			auto* pSourceFile = *pSourceFileRef;
			X_ASSERT_NOT_NULL(pSourceFile);

			if (pSourceFile->getSourceCrc32() == crc32) {
				return pSourceFile;
			}

			// prevent read's while re reparse the data.
			core::ScopedLock<SourceFile::LockType> lock(pSourceFile->lock);

			pSourceFile->getIncludeArr().clear();

			// we should clear refrences, they may of changed.
			pSourceFile->removeRefrences();

			process(pSourceFile);

			return pSourceFile;
		}

		X_ASSERT(arena_->isThreadSafe(), "Arena must be thread safe")();

		auto* pSourceFile = X_NEW(SourceFile, &sourcePoolArena_, "SourceFile")(namelc, arena_);

		process(pSourceFile);

		*pSourceFileRef = pSourceFile;
		return pSourceFile;
	}

	SourceFile* SourceBin::sourceForName(const core::string& name)
	{
		core::CriticalSection::ScopedLock lock(cs_);

		auto it = source_.find(name);
		if (it != source_.end()) {
			return it->second;
		}

		return nullptr;
	}

	void SourceBin::parseIncludesAndPrePro_r(SourceFile* pSourceFile, SourceRefArr& includedFiles, bool reload)
	{
		const auto& fileData = pSourceFile->getFileData();
		const char* pBegin = reinterpret_cast<const char*>(fileData.begin());
		const char* pEnd = reinterpret_cast<const char*>(fileData.end());

		// any includes to process?
		if (core::strUtil::Find(pBegin, pEnd, "#include") == nullptr) {
			return;
		}

		core::XLexer lexer(pBegin, pEnd);
		core::XLexToken token;
		core::string fileName;

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
							fileName.assign(token.begin(), token.end());
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

						if (fileName == pSourceFile->getName())
						{
							X_WARNING("Shader", "Recursive #include in: \"%s\" line: %i",
								pSourceFile->getName().c_str(), token.GetLine());
							return;
						}

						// load it PLZ.
						SourceFile* pChildSourceFile = loadRawSourceFile(fileName, reload);
						if (pChildSourceFile)
						{
							// is this file already included in the tree?
							if (std::find(includedFiles.begin(), includedFiles.end(), pChildSourceFile) == includedFiles.end())
							{
								// check if for includes.
								parseIncludesAndPrePro_r(pChildSourceFile, includedFiles);

								// add the include files crc to this one.
								// only after parsing for child includes so that
								// they are included.
								uint32_t mergedCrc = pCrc32_->Combine(
									pSourceFile->getSourceCrc32(),
									pChildSourceFile->getSourceCrc32(),
									safe_static_cast<uint32_t>(pChildSourceFile->getFileData().size())
								);

								pSourceFile->setSourceCrc32(mergedCrc);
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
											X_ERROR("Shader", "invalid InputLayout prepro in shader: %s value: %s",
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
		core::CriticalSection::ScopedLock lock(cs_);

		X_LOG0("Shader", "--------- ^8Shader Sources(%" PRIuS ")^7 ----------", source_.size());

		auto printfunc = [](const SourceFile* pSource) {
			X_LOG0("Shader", "Name: ^2\"%s\"^7 crc: ^10x%08x^7",
				pSource->getName().c_str(),
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

