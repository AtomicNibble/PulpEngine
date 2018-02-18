#pragma once


#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\Module.h>

class EngineApp : public IAssertHandler
{
public:
	EngineApp();
	~EngineApp() X_OVERRIDE;

	bool Init(const wchar_t* sInCmdLine, PLATFORM_HWND hWnd);
	bool ShutDown(void);

private:
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:
	static void Error(const char* pErrorText);

private:
	core::Module::Handle hSystemHandle_;
	ICore* pICore_;
};
