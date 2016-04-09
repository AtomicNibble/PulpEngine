#pragma once


#include <ICore.h>
#include <Platform\Console.h>
#include <Platform\TrayIcon.h>

class EngineApp : public IAssertHandler, public core::TrayIcon
{
public:
	EngineApp();
	~EngineApp() X_OVERRIDE;

	bool Init(const wchar_t* sInCmdLine, core::Console& Console);
	bool ShutDown(void);
	bool PumpMessages(void);

private:
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

private:

	virtual LRESULT OnTrayCmd(WPARAM wParam, LPARAM lParam) X_FINAL;

private:
	bool run_;

	HMODULE hSystemHandle_;
	ICore* pICore_;
};

