#include "stdafx.h"
#include "TableDump.h"
#include "wrapper\util.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(script)

XScriptTableDumpConsole::XScriptTableDumpConsole()
{
}

XScriptTableDumpConsole::~XScriptTableDumpConsole()
{
}

void XScriptTableDumpConsole::onElementFound(const char* pName, Type::Enum type)
{
    X_LOG0("Script", "\"%s\" ^6%s", pName, Type::ToString(type));
}

void XScriptTableDumpConsole::onElementFound(int idx, Type::Enum type)
{
    X_LOG0("Script", "%i ^6%s", idx, Type::ToString(type));
}

// --------------------------------------------------

XRecursiveLuaDumpToFile::XRecursiveLuaDumpToFile(core::XFile& file) :
    file_(file)
{
    size_ = 0;
}

XRecursiveLuaDumpToFile::~XRecursiveLuaDumpToFile()
{
}

void XRecursiveLuaDumpToFile::onElement(int level, const char* sKey, int nKey, ScriptValue& value)
{
    if (sKey) {
        size_ += core::strUtil::strlen(sKey) + 1;
    }
    else {
        size_ += sizeof(int);
    }

    switch (value.getType()) {
        case Type::Boolean:
            if (value.bool_)
                file_.printf("[%6d] %s %s=true\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey));
            else
                file_.printf("[%6d] %s %s=false\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey));
            break;
        case Type::Handle:
            file_.printf("[%6d] %s %s=%p\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey), value.pPtr_);
            break;
        case Type::Number:
            file_.printf("[%6d] %s %s=%g\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey), value.number_);
            break;
        case Type::String:
            file_.printf("[%6d] %s %s=%s\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey), value.str_.pStr);
            size_ += value.str_.len + 1;
            break;
            //case ANY_TTABLE:
        case Type::Function:
            file_.printf("[%6d] %s %s()\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey));
            break;
            //	case Type::USERDATA:
            //	fprintf(file, "[%6d] %s [userdata] %s\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey));
            //	break;
        case Type::Vector:
            file_.printf("[%6d] %s %s=%g,%g,%g\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey), value.vec3_.x, value.vec3_.y, value.vec3_.z);
            size_ += sizeof(Vec3f);
            break;
    }
}

void XRecursiveLuaDumpToFile::onBeginTable(int level, const char* sKey, int nKey)
{
    file_.printf("[%6d] %s %s = {\n", size_, GetOffsetStr(level), GetKeyStr(sKey, nKey));
}

void XRecursiveLuaDumpToFile::onEndTable(int level)
{
    file_.printf("[%6d] %s }\n", size_, GetOffsetStr(level));
}

const char* XRecursiveLuaDumpToFile::GetOffsetStr(int level)
{
    if (level > sizeof(levelOffset_) - 1) {
        level = sizeof(levelOffset_) - 1;
    }
    memset(levelOffset_, '\t', level);
    levelOffset_[level] = 0;
    return levelOffset_;
}

const char* XRecursiveLuaDumpToFile::GetKeyStr(const char* pKey, int key)
{
    if (pKey) {
        return pKey;
    }

    sprintf(keyStr_, "[%02d]", key);
    return keyStr_;
}

bool dumpStateToFile(lua_State* L, core::string_view fileName)
{
    X_LUA_CHECK_STACK(L);

    core::Path<char> path(fileName.begin(), fileName.end());
    path.setExtension(".txt");

    core::XFileScoped file;
    if (!file.openFile(path, core::FileFlag::WRITE | core::FileFlag::RECREATE)) {
        return false;
    }

    X_LOG0("Script", "Dumping state to: \"%.*s\"", fileName.length(), fileName.data());

    XRecursiveLuaDumpToFile sink(*file.GetFile());

    lua::recursiveTableDump(L, LUA_GLOBALSINDEX, 0, &sink);

#if 0
	X_LUA_CHECK_STACK(L);

	for (std::set<XScriptTable*>::iterator it = XScriptTable::s_allTables_.begin();
		it != XScriptTable::s_allTables_.end(); ++it)
	{
		XScriptTable* pTable = *it;

		pTable->PushRef();
		RecursiveTableDump(L, lua_gettop(L), 1, &sink, tables);
		lua_pop(L, 1);
		sink.OnEndTable(0);
	}
#endif

    return false;
}

X_NAMESPACE_END