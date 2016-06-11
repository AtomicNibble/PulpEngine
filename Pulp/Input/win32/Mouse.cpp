#include "stdafx.h"
#include "Mouse.h"
#include "Win32Input.h"

#include "InputCVars.h"
#include "IConsole.h"

X_NAMESPACE_BEGIN(input)

InputSymbol*	XMouse::Symbol_[MAX_MOUSE_SYMBOLS] = { 0 };


XMouse::XMouse(XWinInput& input) :
	XInputDeviceWin32(input, "mouse")
{
	deviceType_ = InputDeviceType::MOUSE;
	mouseWheel_ = 0.f;
}

XMouse::~XMouse()
{

}


///////////////////////////////////////////
bool XMouse::Init()
{
	X_ASSERT_NOT_NULL(g_pInputCVars);

	RAWINPUTDEVICE Mouse;
	Mouse.hwndTarget = 0;
	Mouse.usUsagePage = 0x1;
	Mouse.usUsage = 0x2;
	Mouse.dwFlags = 0;

	bool res = true;

	if (!RegisterRawInputDevices(&Mouse, 1, sizeof(RAWINPUTDEVICE)))
	{
		core::lastError::Description Dsc;
		X_ERROR("Mouse", "Failed to register mouse: %s", core::lastError::ToString(Dsc));
		res = false;
	}

	// Load the user setting.
	UINT LinesToScroll;
	if (SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &LinesToScroll, 0))
	{
		ADD_CVAR_REF("mouse_scroll_linenum", g_pInputCVars->scrollLines, LinesToScroll, 0, 1920,
			core::VarFlag::SYSTEM, "The number of lines to scroll at a time");
	}
	else
	{
		X_WARNING("Mouse", "Failed to retive sys scroll settings, defaulting to: %i", g_pInputCVars->scrollLines);
	}

	IInput& input = GetIInput();

	// init the symbols
#define DEFINE_SYM(id,name) \
	Symbol_[id - KeyId::INPUT_MOUSE_BASE] = input.DefineSymbol(InputDeviceType::MOUSE, id, name);

	DEFINE_SYM(KeyId::MOUSE_LEFT, "MOUSE LEFT");
	DEFINE_SYM(KeyId::MOUSE_MIDDLE, "MOUSE_MIDDLE");
	DEFINE_SYM(KeyId::MOUSE_RIGHT, "MOUSE_RIGHT");
	DEFINE_SYM(KeyId::MOUSE_AUX_1, "MOUSE_AUX_1");
	DEFINE_SYM(KeyId::MOUSE_AUX_2, "MOUSE_AUX_2");
	DEFINE_SYM(KeyId::MOUSE_AUX_3, "MOUSE_AUX_3");
	DEFINE_SYM(KeyId::MOUSE_AUX_4, "MOUSE_AUX_4");
	DEFINE_SYM(KeyId::MOUSE_AUX_5, "MOUSE_AUX_5");

	DEFINE_SYM(KeyId::MOUSE_WHEELUP, "MOUSE_AUX_5");
	DEFINE_SYM(KeyId::MOUSE_WHEELDOWN, "MOUSE_AUX_5");

	DEFINE_SYM(KeyId::MOUSE_X, "MOUSE_X");
	DEFINE_SYM(KeyId::MOUSE_Y, "MOUSE_Y");
	DEFINE_SYM(KeyId::MOUSE_Z, "MOUSE_Z");

	DEFINE_SYM(KeyId::MOUSE_WHEELDOWN, "MOUSE_WHEELDOWN");
	DEFINE_SYM(KeyId::MOUSE_WHEELUP, "MOUSE_WHEELUP");


	return res;
}

void XMouse::ShutDown()
{
	RAWINPUTDEVICE Mouse;
	Mouse.hwndTarget = 0;
	Mouse.usUsagePage = 0x1;
	Mouse.usUsage = 0x2;
	Mouse.dwFlags = RIDEV_REMOVE;

	if (!RegisterRawInputDevices(&Mouse, 1, sizeof(RAWINPUTDEVICE)))
	{
		core::lastError::Description Dsc;
		X_ERROR("Mouse", "Failed to unregister mouse: %s", core::lastError::ToString(Dsc));
	}
}

///////////////////////////////////////////
void XMouse::Update(bool focus)
{
	X_ASSERT_UNREACHABLE();
	X_UNUSED(focus);
}

void XMouse::ProcessInput(const uint8_t* pData)
{
	const RAWMOUSE& mouse = *reinterpret_cast<const RAWMOUSE*>(pData);
	ProcessMouseData(mouse);
}


void XMouse::PostEvent(InputSymbol* pSymbol)
{
	if (pSymbol)
	{
		InputEvent event;
		pSymbol->AssignToEvent(event, GetIInput().GetModifiers());
		GetIInput().PostInputEvent(event);
	}
}

void XMouse::PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState)
{
	if (pSymbol->state != InputState::RELEASED && newState == InputState::RELEASED)
	{
		pSymbol->state = newState;
		pSymbol->value = 0.0f;
	}
	else if (pSymbol->state == InputState::RELEASED && newState == InputState::PRESSED)
	{
		pSymbol->state = newState;
		pSymbol->value = 1.0f;
	}
	else
	{
		return;
	}

	PostEvent(pSymbol);
}


void XMouse::OnButtonDown(KeyId::Enum id)
{
	InputSymbol* pSymbol = GetSymbol(id);

	pSymbol->value = 1.f;
	pSymbol->state = InputState::PRESSED;

	if (g_pInputCVars->input_debug)
	{
		X_LOG0("Mouse", "Button Down: \"%s\"", pSymbol->name.c_str());
	}

	PostEvent(pSymbol);
}

void XMouse::OnButtonUP(KeyId::Enum id)
{
	InputSymbol* pSymbol = GetSymbol(id);

	pSymbol->value = 0.f;
	pSymbol->state = InputState::RELEASED;

	if (g_pInputCVars->input_debug)
	{
		X_LOG0("Mouse", "Button Up: \"%s\"", pSymbol->name.c_str());
	}

	PostEvent(pSymbol);
}

void XMouse::ProcessMouseData(const RAWMOUSE& mouse)
{

	// i think i can get all info from raw input tbh.
	// get rekt.
	if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
	{
		OnButtonDown(KeyId::MOUSE_LEFT);
	}
	else if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
	{
		OnButtonUP(KeyId::MOUSE_LEFT);
	}

	// Right Mouse
	if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
	{
		OnButtonDown(KeyId::MOUSE_RIGHT);
	}
	else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
	{
		OnButtonUP(KeyId::MOUSE_RIGHT);
	}

	// Middle Mouse
	if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
	{
		OnButtonDown(KeyId::MOUSE_MIDDLE);
	}
	else if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
	{
		OnButtonUP(KeyId::MOUSE_MIDDLE);
	}

	// aux 4
	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
	{
		OnButtonDown(KeyId::MOUSE_AUX_4);
	}
	else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
	{
		OnButtonUP(KeyId::MOUSE_AUX_4);
	}

	// aux 5
	if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
	{
		OnButtonDown(KeyId::MOUSE_AUX_5);
	}
	else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
	{
		OnButtonUP(KeyId::MOUSE_AUX_5);
	}

	if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
	{
		if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
			X_WARNING("Mouse", "Absolute scroll is not yet supported.");
		}

		mouseWheel_ = static_cast<float>(static_cast<short>(mouse.usButtonData));

		// we post scrool up and down once.
		InputState::Enum newState;

		// if the value is positive post a single up event.
		newState = (mouseWheel_ > 0.0f) ? InputState::PRESSED : InputState::RELEASED;

		if (g_pInputCVars->input_debug && newState == InputState::PRESSED)
		{
			X_LOG0("Mouse", "ScrollUp(%g)", mouseWheel_);
		}

		PostOnlyIfChanged(GetSymbol(KeyId::MOUSE_WHEELUP), newState);

		newState = (mouseWheel_ < 0.0f) ? InputState::PRESSED : InputState::RELEASED;

		if (g_pInputCVars->input_debug && newState == InputState::PRESSED)
		{
			X_LOG0("Mouse", "ScrollDown(%g)", mouseWheel_);
		}

		PostOnlyIfChanged(GetSymbol(KeyId::MOUSE_WHEELDOWN), newState);

		// we post scrool direction events.
		InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_Z);

		pSymbol->value = mouseWheel_;
		pSymbol->state = InputState::CHANGED;


		PostEvent(pSymbol);
	}

	if (mouse.usFlags == MOUSE_MOVE_RELATIVE)
	{
		if (mouse.lLastX != 0)
		{
			InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_X);

			pSymbol->value = (float)mouse.lLastX;
			pSymbol->state = InputState::CHANGED;

			if (g_pInputCVars->input_mouse_pos_debug)
			{
				X_LOG0("Mouse", "posX: \"%g\"", pSymbol->value);
			}

			PostEvent(pSymbol);
		}
		if (mouse.lLastY != 0)
		{
			InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_Y);

			pSymbol->value = (float)mouse.lLastY;
			pSymbol->state = InputState::CHANGED;

			if (g_pInputCVars->input_mouse_pos_debug)
			{
				X_LOG0("Mouse", "posY: \"%g\"", pSymbol->value);
			}

			PostEvent(pSymbol);
		}
	}

	// input_mouse_pos_debug
}

///////////////////////////////////////////
bool XMouse::SetExclusiveMode(bool value)
{
	X_ASSERT_NOT_IMPLEMENTED();
	X_UNUSED(value);
	return true;
}

X_NAMESPACE_END