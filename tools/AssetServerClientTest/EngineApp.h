#pragma once


#include <ICore.h>
#include <Platform\Console.h>

class EngineApp
{
public:
	EngineApp();
	~EngineApp();

	bool Init(const wchar_t* sInCmdLine, core::Console& Console);
	bool ShutDown(void);
	int	MainLoop(void);

private:

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool PumpMessages();

private:
	HMODULE hSystemHandle_;
	ICore* pICore_;
};

