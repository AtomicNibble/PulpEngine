#pragma once


#include <ICore.h>
#include <Platform\Console.h>

#define WIN_ENGINE_WINDOW_CLASSNAME "WinCatEngine"

class EngineApp
{
public:
	EngineApp();
	~EngineApp();

	bool Init(const char* sInCmdLine, core::Console& Console);
	int	MainLoop();

private:

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Main Engine loop.
	bool PumpMessages();
private:
	HMODULE hSystemHandle_;
	ICore* pICore_;
};

