#include "stdafx.h"
#include "Win32Input.h"
#include <ICore.h>

#include "Mouse.h"
#include "Keyboard.h"

#include "InputDeviceWin32.h"

#include <Util\LastError.h>

#include "InputCVars.h"

X_NAMESPACE_BEGIN(input)


XWinInput* XWinInput::This = 0;

XWinInput::XWinInput(ICore* pSystem, HWND hwnd) : XBaseInput()
{
	X_UNUSED(pSystem);

	isWow64_ = FALSE;
	m_hwnd = hwnd;
	m_prevWndProc = 0;
	This = this;

	Devices_.reserve(2);
};

XWinInput::~XWinInput()
{
	This = nullptr;
}


bool XWinInput::Init(void)
{
	XBaseInput::Init();

	// work out if WOW64.
	if (!::IsWow64Process(::GetCurrentProcess(), &isWow64_)) {
		core::lastError::Description Dsc;
		X_ERROR("Input", "Wow64 check failed. Error: %s", core::lastError::ToString(Dsc));
		return false;
	}

	// o baby!
	if (!AddInputDevice(X_NEW_ALIGNED(XKeyboard, g_InputArena, "Keyboard", 8)(*this))) {
		X_ERROR("Input", "Failed to add keyboard input device");
		return false;
	}

	if (!AddInputDevice(X_NEW_ALIGNED(XMouse, g_InputArena, "Mouse", 8)(*this))) {
		X_ERROR("Input", "Failed to add mouse input device");
		return false;
	}

	ClearKeyState();
	return true;
}


bool XWinInput::AddInputDevice(IInputDevice* pDevice)
{
	X_UNUSED(pDevice);
	return false;
}
bool XWinInput::AddInputDevice(XInputDeviceWin32* pDevice)
{
	return XBaseInput::AddInputDevice(pDevice);
}


void XWinInput::Update(bool bFocus)
{
	X_PROFILE_BEGIN("Win32RawInput", core::ProfileSubSys::INPUT);

	PostHoldEvents();

	hasFocus_ = bFocus;

	const size_t BufSize = 0x80;
	const size_t HeaderSize = sizeof(RAWINPUTHEADER);

	RAWINPUT X_ALIGNED_SYMBOL(input[BufSize], 8);
	UINT size;
	UINT num;

	for (;;)
	{
		size = sizeof(input);

		num = GetRawInputBuffer(input, &size, HeaderSize);

		if (num == 0)
			break;

		if (g_pInputCVars->input_debug > 1)
		{
			X_LOG0("Input", "Buffer size: %i", num);
		}

		if (num == (UINT)-1)
		{
			core::lastError::Description Dsc;
			X_ERROR("Input", "Failed to get input. Error: %s", core::lastError::ToString(Dsc));
			break;
		}

		PRAWINPUT rawInput = input;

		for (UINT i = 0; i < num; ++i)
		{
			const uint8_t* pData = reinterpret_cast<const uint8_t*>(&rawInput->data);
			// needs to be 16 + 8 aligned.
			if (isWow64_) {
				pData += 8;
			}

			for (TInputDevices::Iterator i = Devices_.begin(); i != Devices_.end(); ++i)
			{
				XInputDeviceWin32* pDevice = (XInputDeviceWin32*)(*i);
				if (pDevice->IsEnabled())
				{
					pDevice->ProcessInput(rawInput->header, pData);
				}
			}

			rawInput = NEXTRAWINPUTBLOCK(rawInput);
		}
		// to clean the buffer
		// DefRawInputProc(&pInput, num, sizeof(RAWINPUTHEADER));
	}


	// send commit event after all input processing for this frame has finished
	InputEvent event;
	event.modifiers = modifiers_;
	event.deviceId = InputDevice::UNKNOWN;
	event.value = 0;
	event.name = "final";
	event.keyId = KeyId::UNKNOWN;
	PostInputEvent(event);

	if (g_pInputCVars->input_debug > 1)
	{
	//	X_LOG0("Input", "-- Frame --");
	}
}


void XWinInput::ShutDown(void)
{
	XBaseInput::ShutDown();


}

void XWinInput::release(void)
{
	X_DELETE(this, g_InputArena);
}

void XWinInput::ClearKeyState()
{
	XBaseInput::ClearKeyState();



}


// reroute to instance function
LRESULT CALLBACK XWinInput::InputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return This->OnInputWndProc(hWnd, message, wParam, lParam);
}

LRESULT XWinInput::OnInputWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INPUT)
	{
		// message filter must filter them like so:
		// while (PeekMessage (&amp;msg, NULL, WM_INPUT + 1, 0xffffffff, PM_REMOVE)
		X_ERROR("Window", "Window is reciving WM_INPUT messages, need to be filterd for input systems.");
	}
	else if (message == WM_CHAR)
	{

	}

	return ::CallWindowProc(m_prevWndProc, hWnd, message, wParam, lParam);
}


X_NAMESPACE_END