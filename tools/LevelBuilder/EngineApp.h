#pragma once


#include <ICore.h>
#include <Platform\Console.h>

#define WIN_ENGINE_WINDOW_CLASSNAME "WinCatEngine"

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

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HMODULE hSystemHandle_;
	ICore* pICore_;
};

