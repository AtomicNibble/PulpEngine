#pragma once


#include <ICore.h>
#include <Platform\Console.h>

class EngineApp : public IAssertHandler
{
public:
	EngineApp();
	~EngineApp() X_OVERRIDE;

	bool Init(const wchar_t* sInCmdLine, core::Console& Console);
	bool ShutDown(void);

private:
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:

private:
	HMODULE hSystemHandle_;
	ICore* pICore_;
};

