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


void XWinInput::Update(core::V2::Job* pInputJob, core::FrameData& frameData)
{
	X_PROFILE_BEGIN("Win32RawInput", core::ProfileSubSys::INPUT);

	PostHoldEvents();

	hasFocus_ = frameData.flags.IsSet(core::FrameFlag::HAS_FOCUS);

	RAWINPUT X_ALIGNED_SYMBOL(input[BUF_NUM], 8);

	UINT size;
	UINT num;

	const bool debug = (g_pInputCVars->input_debug > 1);

	for (;;)
	{
		size = sizeof(input);
		num = GetRawInputBuffer(input, &size, static_cast<UINT>(ENTRY_HDR_SIZE));

		if (num == 0) {
			break;
		}

		if (debug) {
			X_LOG0("Input", "Buffer size: %i", num);
		}
			
		if (num == static_cast<UINT>(-1)) {
			core::lastError::Description Dsc;
			X_ERROR("Input", "Failed to get input. Error: %s", core::lastError::ToString(Dsc));
			break;
		}

		// now lets create jobs for each.
		core::V2::JobSystem& jobSys = *pJobSystem_;
		core::V2::Job* pJob = nullptr;

		PRAWINPUT rawInput = input;
		for (UINT i = 0; i < num; ++i)
		{
			const uint8_t* pData = reinterpret_cast<const uint8_t*>(&rawInput->data);
			// needs to be 16 + 8 aligned.
			if (isWow64_) {
				pData += 8;
			}

			if (rawInput->header.dwType == RIM_TYPEKEYBOARD)
			{
				const RAWMOUSE& mouseData = *reinterpret_cast<const RAWMOUSE*>(pData);

				pJob = jobSys.CreateMemberJobAsChild<XWinInput>(pInputJob, this, &XWinInput::Job_ProcessMouseEvents, mouseData);
				jobSys.Run(pJob);
			}
			else if(rawInput->header.dwType == RIM_TYPEMOUSE)
			{
				const RAWKEYBOARD& keyboardData = *reinterpret_cast<const RAWKEYBOARD*>(pData);

				pJob = jobSys.CreateMemberJobAsChild<XWinInput>(pInputJob, this, &XWinInput::Job_ProcessKeyboardEvents, keyboardData);
				jobSys.Run(pJob);
			}

			rawInput = NEXTRAWINPUTBLOCK(rawInput);
		}

		// since we make copyies of the data for each job.
		// we don't need to wait before asking GetRawInputBuffer again and reusing 'input'
		// we can also leave this function after since all the job's are children and must complete
		// before the base inoput job will be marked as completed.
	}
}


void XWinInput::ClearKeyState()
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


void XWinInput::Job_ProcessMouseEvents(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_ASSERT_NOT_NULL(pMouse_);
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

//	const RAWMOUSE& mouseData = *reinterpret_cast<const RAWMOUSE*>(pData);



}

void XWinInput::Job_ProcessKeyboardEvents(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_ASSERT_NOT_NULL(pKeyBoard_);
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);

//	const RAWKEYBOARD& keyboardData = *reinterpret_cast<const RAWKEYBOARD*>(pData);


}


X_NAMESPACE_END