#pragma once


#ifndef X_SCRIPT_BINDS_H_
#define X_SCRIPT_BINDS_H_

#include <Containers\FixedArray.h>
#include <String\StackString.h>

#include <Util\SmartPointer.h>
#include <Util\ScopedPointer.h>

X_NAMESPACE_BEGIN(script)


class XScriptableBase;

class XScriptBinds
{
	typedef core::FixedArray<core::ScopedPointer<XScriptableBase>, ScriptMoudles::ENUM_COUNT> ScriptModels;

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