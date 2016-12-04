#include "stdafx.h"
#include "TechDefs.h"
#include "TechSetDef.h"

#include "Util\MatUtil.h"

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Hashing\Fnva1Hash.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)


	// ------------------------------------------------------------------------------

	const char* TechSetDefs::INCLUDE_DIR = "include";
	const wchar_t* TechSetDefs::INCLUDE_DIR_W = L"include";


	TechSetDefs::TechSetDefs(core::MemoryArenaBase* arena) :
		arena_(arena),
		techDefs_(arena, 128),
		incSourceMap_(arena, 128)
	{

	}

	TechSetDefs::~TechSetDefs()
	{
		for (const auto& t : techDefs_)
		{
			X_DELETE(t.second, arena_);
		}
	}

	// bool TechSetDefs::getTechCats(TechCatArr& techsOut)
	bool TechSetDefs::getTechCatTypes(MaterialCat::Enum cat, CatTypeArr& typesOut)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);

		core::Path<char> path(TECH_DEFS_DIR);
		path /= MaterialCat::ToString(cat);
		path.toLower();

		if (gEnv->pFileSys->directoryExists(path.c_str()))
		{
			if (loadTechCat(cat, typesOut)) {
				return true;
			}

			X_WARNING("TechSetDefs", "Failed to load tech types for cat: \"%s\"", MaterialCat::ToString(cat));
			return false;
		}

		X_WARNING("TechSetDefs", "Directory for material cat missing: \"%s\"", MaterialCat::ToString(cat));
		return false;
	}

	bool TechSetDefs::getTechDef(const MaterialCat::Enum cat, const char* pName, TechSetDef*& pTechDefOut)
	{
		return getTechDef(cat, core::string(pName), pTechDefOut);
	}

	bool TechSetDefs::getTechDef(const MaterialCat::Enum cat, const core::string& name, TechSetDef*& pTechDefOut)
	{
		core::Path<char> path;
		path /= MaterialCat::ToString(cat);
		path /= name;
		path.toLower();
		path.replaceSeprators();
		path.setExtension(TECH_DEFS_FILE_EXTENSION);

		auto it = techDefs_.find(path);
		if (it != techDefs_.end()) {
			pTechDefOut = it->second;
			return true;
		}

		if (!loadTechDef(path, name)) {
			return false;
		}

		it = techDefs_.find(path);
		X_ASSERT(it != techDefs_.end(), "Must be in map if load passed")();

		pTechDefOut = it->second;
		return true;
	}

	bool TechSetDefs::loadTechCat(MaterialCat::Enum cat, CatTypeArr& typesOut)
	{
		core::Path<char> path(TECH_DEFS_DIR);
		path /= MaterialCat::ToString(cat);
		path.ensureSlash();
		path.appendFmt("*.%s", TECH_DEFS_FILE_EXTENSION);
		path.toLower();

		core::FindFirstScoped find;

		if (find.findfirst(path.c_str()))
		{
			char buf[512];

			core::Path<char> name;

			do
			{
				const auto& fd = find.fileData();

				if (fd.attrib & FILE_ATTRIBUTE_DIRECTORY) {
					continue;
				}

				// remove the extension.
				name.set(core::strUtil::Convert(fd.name, buf));
				name.removeExtension();

				typesOut.emplace_back(name.c_str());
			} 
			while (find.findNext());
		}

		return true;
	}

	bool TechSetDefs::loadTechDef(const core::Path<char>& path, const core::string& name)
	{
		FileBuf fileData(arena_);

		if (!loadFile(path, fileData)) {
			return false;
		}

		TechSetDef* pTechDef = X_NEW(TechSetDef, arena_, "TechDef")(name, arena_);

		TechSetDef::OpenIncludeDel incDel;
		incDel.Bind<TechSetDefs, &TechSetDefs::includeCallback>(this);

		if (!pTechDef->parseFile(fileData, incDel)) {
			X_ERROR("TechSetDefs", "Failed to load: \"%s\"", name.c_str());
			return false;
		}

		techDefs_.insert(TechSetDefMap::value_type(path, pTechDef));
		return true;
	}

	void TechSetDefs::clearIncSrcCache(void)
	{
		incSourceMap_.clear();
	}

	bool TechSetDefs::loadFile(const core::Path<char>& path, FileBuf& bufOut)
	{
		core::XFileScoped file;
		core::fileModeFlags mode = core::fileMode::READ | core::fileMode::SHARE;

		core::Path<char> fullPath(TECH_DEFS_DIR);
		fullPath /= path;

		if (!file.openFile(fullPath.c_str(), mode)) {
			X_ERROR("TechSetDefs", "Failed to open file: \"%s\"", path.c_str());
			return false;
		}

		const size_t fileSize = safe_static_cast<size_t>(file.remainingBytes());

		bufOut.resize(fileSize);

		if (file.read(bufOut.data(), fileSize) != fileSize) {
			X_ERROR("TechSetDefs", "Failed to read file data");
			return false;
		}

		return true;
	}

	bool TechSetDefs::includeCallback(core::XLexer& lex, core::string& name, bool useIncludePath)
	{
		core::Path<char> path;

		if (useIncludePath) {
			path = INCLUDE_DIR;
			path /= name;
		}
		else {
			path = name;
		}

		path.setExtension(TECH_DEFS_FILE_EXTENSION);

		auto it = incSourceMap_.find(path);
		if (it == incSourceMap_.end())
		{
			// we need to load it.
			FileBuf fileData(arena_);

			if (!loadFile(path, fileData)) {
				return false;
			}

			auto insertedIt = incSourceMap_.insert(SourceMap::value_type(path, std::move(fileData)));
			it = insertedIt.first;
		}

		FileBuf& buf = it->second;

		return lex.SetMemory(buf.begin(), buf.end(), core::string(path.c_str()));
	}



X_NAMESPACE_END
