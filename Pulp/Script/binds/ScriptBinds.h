#pragma once


#ifndef X_SCRIPT_BINDS_H_
#define X_SCRIPT_BINDS_H_

#include <Containers\FixedArray.h>

#include <Util\SmartPointer.h>
#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(script)


class XScriptBinds
{
	typedef core::FixedArray<core::UniquePointer<XScriptableBase>, Moudles::ENUM_COUNT> ScriptModels;

public:
	XScriptBinds();
	~XScriptBinds();

	void Init(IScriptSys* pScriptSystem, ICore* pCore);
	void Shutdown(void);

private:

	ScriptModels modules_;
};


X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_H_