#include "stdafx.h"
#include "Win32Input.h"
#include <ICore.h>

#include "Mouse.h"
#include "Keyboard.h"

#include "InputDeviceWin32.h"

#include <Util\LastError.h>
#include "InputCVars.h"

#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(input)



XWinInput::XWinInput(ICore* pSystem, HWND hWnd) :
	XBaseInput(),
	isWow64_(FALSE),
	hWnd_(hWnd),
	pKeyBoard_(nullptr),
	pMouse_(nullptr),
	pJobSystem_(nullptr)
{
	pJobSystem_ = pSystem->GetJobSystem();

	devices_.reserve(2);
};

XWinInput::~XWinInput()
{
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

	pKeyBoard_ = X_NEW_ALIGNED(XKeyboard, g_InputArena, "Keyboard", 8)(*this);
	pMouse_ = X_NEW_ALIGNED(XMouse, g_InputArena, "Mouse", 8)(*this);


	// o baby!
	if (!AddInputDevice(pKeyBoard_)) {
		X_ERROR("Input", "Failed to add keyboard input device");
		return false;
	}

	if (!AddInputDevice(pMouse_)) {
		X_ERROR("Input", "Failed to add mouse input device");
		return false;
	}

	ClearKeyState();
	return true;
}



void XWinInput::PostInit(void)
{
	XBaseInput::PostInit();
}


void XWinInput::ShutDown(void)
{
	XBaseInput::ShutDown();
}

void XWinInput::release(void)
{
	X_DELETE(this, g_InputArena);
}


void XWinInput::Job_Update(core::V2::JobSystem& jobSys, core::V2::Job* pInputJob, core::FrameData& frameData)
{
	X_PROFILE_BEGIN("Win32RawInput", core::ProfileSubSys::INPUT);
	X_ASSERT_NOT_NULL(pMouse_);
	X_ASSERT_NOT_NULL(pKeyBoard_);
	X_UNUSED(jobSys);
	X_UNUSED(pInputJob);


	AddHoldEvents(frameData.input);

	hasFocus_ = frameData.flags.IsSet(core::FrameFlag::HAS_FOCUS);

	RAWINPUT X_ALIGNED_SYMBOL(input[BUF_NUM], 8);

	UINT size;
	size_t num;

	const bool debug = (g_pInputCVars->input_debug > 1);
	const bool mouseEnabled = pMouse_->IsEnabled();
	const bool keyboardEnabled = pKeyBoard_->IsEnabled();

	for (;;)
	{
		size = sizeof(input);
		num = GetRawInputBuffer(input, &size, static_cast<UINT>(ENTRY_HDR_SIZE));

		// log size even if empty
		if (debug) {
			X_LOG0("Input", "Buffer size: %i threadId: 0x%x", num, core::Thread::GetCurrentID());
		}

		if (num == 0) {
			break;
		}
			
		if (num == static_cast<UINT>(-1)) {
			core::lastError::Description Dsc;
			X_ERROR("Input", "Failed to get input. Error: %s", core::lastError::ToString(Dsc));
			break;
		}

		auto& inputData = frameData.input;

		const size_t eventSpace = inputData.events.capacity() - inputData.events.size();
		if (eventSpace < num) {
			X_WARNING("Input", "Input frame buffer full ignoring %i events", num - eventSpace);
			num = eventSpace;
		}

		PRAWINPUT rawInput = input;
		for (UINT i = 0; i < num; ++i)
		{
			const uint8_t* pData = reinterpret_cast<const uint8_t*>(&rawInput->data);
			// needs to be 16 + 8 aligned.
			if (isWow64_) {
				pData += 8;
			}

			if (rawInput->header.dwType == RIM_TYPEMOUSE && mouseEnabled)
			{
				pMouse_->ProcessInput(pData, inputData);
			}
			else if(rawInput->header.dwType == RIM_TYPEKEYBOARD && keyboardEnabled)
			{
				pKeyBoard_->ProcessInput(pData, inputData);
			}

			rawInput = NEXTRAWINPUTBLOCK(rawInput);
		}
	}
}


void XWinInput::ClearKeyState(void)
{
	XBaseInput::ClearKeyState();
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



X_NAMESPACE_END