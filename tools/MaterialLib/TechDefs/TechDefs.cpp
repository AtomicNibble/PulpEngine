#include "stdafx.h"
#include "TechDefs.h"
#include "TechSetDef.h"

#include "Util\MatUtil.h"

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Hashing\Fnva1Hash.h>
#include <Util\UniquePointer.h>
#include <Time\StopWatch.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)

namespace techset
{
    // ------------------------------------------------------------------------------

    namespace
    {
        TechSetDef* const INVALID_TECH_SET_DEF = reinterpret_cast<TechSetDef*>(std::numeric_limits<uintptr_t>::max());

    } // namespace

    const char* TechSetDefs::INCLUDE_DIR = "include";
    const wchar_t* TechSetDefs::INCLUDE_DIR_W = L"include";

    TechSetDefs::TechSetDefs(core::MemoryArenaBase* arena) :
        arena_(arena),
        techDefs_(arena, 128),
        incSourceMap_(arena, 64)
    {
    }

    TechSetDefs::~TechSetDefs()
    {
        for (const auto& t : techDefs_) {

            if (t.second == INVALID_TECH_SET_DEF) {
                X_WARNING("TechSetDefs", "Skipping delete of invalid techset: \"%s\"", t.first.c_str());
                continue;
            }

            X_DELETE(t.second, arena_);
        }
    }

    bool TechSetDefs::techCatPresent(MaterialCat::Enum cat)
    {
        X_ASSERT_NOT_NULL(gEnv->pFileSys);

        core::Path<char> path;
        getTechCatPath(cat, path);

        return gEnv->pFileSys->directoryExists(path);
    }

    // bool TechSetDefs::getTechCats(TechCatArr& techsOut)
    bool TechSetDefs::getTechCatTypes(MaterialCat::Enum cat, CatTypeArr& typesOut)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFileSys);

        core::Path<char> path;
        getTechCatPath(cat, path);

        if (gEnv->pFileSys->directoryExists(path)) {
            if (loadTechCat(cat, typesOut)) {
                return true;
            }

            X_WARNING("TechSetDefs", "Failed to load tech types for cat: \"%s\"", MaterialCat::ToString(cat));
            return false;
        }

        X_WARNING("TechSetDefs", "Directory for material cat missing: \"%s\"", MaterialCat::ToString(cat));
        return false;
    }

    TechSetDef* TechSetDefs::getTechDef(const MaterialCat::Enum cat, const core::string& name)
    {
        core::Path<char> path;
        path /= MaterialCat::ToString(cat);
        path /= name;
        path.toLower();
        path.replaceSeprators();
        path.setExtension(TECH_DEFS_FILE_EXTENSION);

        TechSetDef** pTechDefRef = nullptr;
        bool loaded = true;

        {
            core::CriticalSection::ScopedLock lock(cacheLock_);

            auto it = techDefs_.find(path);
            if (it != techDefs_.end()) {
                pTechDefRef = &it->second;
            }
            else {
                loaded = false;
                auto insertIt = techDefs_.insert(std::make_pair(path, nullptr));
                pTechDefRef = &insertIt.first->second;
            }
        }

        if (loaded) {
            while (*pTechDefRef == nullptr) {
                core::Thread::sleep(1);
            }

            if (*pTechDefRef == INVALID_TECH_SET_DEF) {
                return nullptr;
            }

            return *pTechDefRef;
        }

        auto* pTechDef = loadTechDef(path, cat, name);
        if (!pTechDef) {
            *pTechDefRef = INVALID_TECH_SET_DEF;
            return nullptr;
        }

        *pTechDefRef = pTechDef;
        return pTechDef;
    }

    bool TechSetDefs::loadTechCat(MaterialCat::Enum cat, CatTypeArr& typesOut)
    {
        core::Path<char> path;
        getTechCatPath(cat, path);
        path.ensureSlash();
        path.appendFmt("*.%s", TECH_DEFS_FILE_EXTENSION);
        path.toLower();

        core::FindFirstScoped find;

        if (find.findfirst(path)) {

            do {
                auto& fd = find.fileData();
                if (fd.attrib.IsSet(core::FindData::AttrFlag::DIRECTORY)) {
                    continue;
                }

                // remove the extension.
                fd.name.removeExtension();

                typesOut.emplace_back(fd.name.c_str());
            } while (find.findNext());
        }

        return true;
    }

    void TechSetDefs::getTechCatPath(MaterialCat::Enum cat, core::Path<char>& path)
    {
        path.clear();
        path.append(assetDb::AssetType::ToString(assetDb::AssetType::TECHDEF));
        path.append('s', 1);
        path.toLower();
        path /= MaterialCat::ToString(cat);
        path.toLower();
    }

    bool TechSetDefs::loadFile(const core::Path<char>& path, FileBuf& bufOut)
    {
        core::XFileScoped file;
        core::FileFlags mode = core::FileFlag::READ | core::FileFlag::SHARE;

        core::Path<char> fullPath;
        fullPath.append(assetDb::AssetType::ToString(assetDb::AssetType::TECHDEF));
        fullPath.append('s', 1);
        fullPath.toLower();
        fullPath /= path;

        if (!file.openFile(fullPath, mode)) {
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

    TechSetDef* TechSetDefs::loadTechDef(const core::Path<char>& path, MaterialCat::Enum cat, const core::string& name)
    {
        FileBuf fileData(arena_);

        if (!loadFile(path, fileData)) {
            return nullptr;
        }

        core::UniquePointer<TechSetDef> techDef = core::makeUnique<TechSetDef>(arena_, name, arena_);

        TechSetDef::OpenIncludeDel incDel;
        incDel.Bind<TechSetDefs, &TechSetDefs::includeCallback>(this);

        if (!techDef->parseFile(fileData, incDel)) {
            X_ERROR("TechSetDefs", "Failed to load: \"%s:%s\"", MaterialCat::ToString(cat), name.c_str());
            return nullptr;
        }

        return techDef.release();
    }

    void TechSetDefs::clearIncSrcCache(void)
    {
        core::CriticalSection::ScopedLock lock(sourceCacheLock_);

        incSourceMap_.clear();
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

        SourceMap::iterator it;
        {
            core::CriticalSection::ScopedLock lock(sourceCacheLock_);

            it = incSourceMap_.find(path);
            if (it != incSourceMap_.end()) {
                FileBuf& buf = it->second;
                return lex.SetMemory(buf.begin(), buf.end(), core::string(path.c_str()));
            }
        }

        // load it.
        // we allow duplicate loads currently.
        FileBuf fileData(arena_);
        if (!loadFile(path, fileData)) {
            return false;
        }

        {
            core::CriticalSection::ScopedLock lock(sourceCacheLock_);

            auto insertedIt = incSourceMap_.insert(SourceMap::value_type(path, std::move(fileData)));
            it = insertedIt.first;
        }

        FileBuf& buf = it->second;
        return lex.SetMemory(buf.begin(), buf.end(), core::string(path.c_str()));
    }

} // namespace techset

X_NAMESPACE_END
