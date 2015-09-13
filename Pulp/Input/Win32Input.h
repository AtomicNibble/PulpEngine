#pragma once

#ifndef _X_INPUT_WIN32_H_
#define _X_INPUT_WIN32_H_

#include "BaseInput.h"

struct	ICore;

X_NAMESPACE_BEGIN(input)

class XInputDeviceWin32;

class XWinInput : public XBaseInput
{
public:
	XWinInput(ICore* pCore, HWND hWnd);
	virtual ~XWinInput() X_OVERRIDE;

	// IInput overrides
	virtual bool	Init(void) X_OVERRIDE;
	virtual void	Update(bool bFocus) X_OVERRIDE;
	virtual void	ShutDown(void) X_OVERRIDE;
	virtual void	release(void) X_OVERRIDE;

	virtual void	ClearKeyState(void) X_OVERRIDE;
	// ~IInput

	virtual bool    AddInputDevice(IInputDevice* pDevice) X_OVERRIDE;
	virtual bool	AddInputDevice(XInputDeviceWin32* pDevice);


	HWND			GetHWnd(void) const	{ return hwnd_; }

private:
	// Window procedure handling
	static LRESULT CALLBACK InputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT OnInputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


private:
	BOOL					isWow64_;
	HWND					hwnd_;
	WNDPROC					prevWndProc_;
	static XWinInput*		s_pThis;
};

X_NAMESPACE_END

#endif // !_X_INPUT_WIN32_H_
