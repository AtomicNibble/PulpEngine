#pragma once

#include <IFileSys.h>

#include "util.h"

X_NAMESPACE_BEGIN(script)

namespace lua
{

	struct XRecursiveLuaDumpToFile : public IRecursiveLuaDump
	{
		XRecursiveLuaDumpToFile(core::XFileScoped& file);
		~XRecursiveLuaDumpToFile() X_FINAL;

		virtual void onElement(int level, const char* sKey, int nKey, ScriptValue& value) X_FINAL;
		void onBeginTable(int level, const char* sKey, int nKey) X_FINAL;
		void onEndTable(int level) X_FINAL;

	private:
		const char* GetOffsetStr(int level);
		const char* GetKeyStr(const char* pKey, int key);

	private:
		core::XFileScoped& file_;

		char levelOffset_[1024];
		char keyStr_[32];
		size_t size_;
	};

	bool dumpStateToFile(lua_State* L, const char* pName);

} // namespace lua

X_NAMESPACE_END