#include "stdafx.h"
#include "Win32Input.h"
#include <ICore.h>

#include "Mouse.h"
#include "Keyboard.h"

#include "InputDeviceWin32.h"
#include "InputCVars.h"

#include <Util\LastError.h>
#include <Platform\Window.h>

X_NAMESPACE_BEGIN(input)

XWinInput::XWinInput(core::MemoryArenaBase* arena, HWND hWnd) :
    XBaseInput(arena),
    isWow64_(FALSE),
    hWnd_(hWnd),
    pKeyBoard_(nullptr),
    pMouse_(nullptr)
{
    devices_.reserve(2);
};

XWinInput::~XWinInput()
{
}

void XWinInput::registerVars(void)
{
    XBaseInput::registerVars();

}

void XWinInput::registerCmds(void)
{
    XBaseInput::registerCmds();

}

bool XWinInput::init(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("InputInit", core::profiler::SubSys::INPUT);

    XBaseInput::init();

    // work out if WOW64.
    if (!::IsWow64Process(::GetCurrentProcess(), &isWow64_)) {
        core::lastError::Description Dsc;
        X_ERROR("Input", "Wow64 check failed. Error: %s", core::lastError::ToString(Dsc));
        return false;
    }

    auto keyBoard = core::makeUnique<XKeyboard>(g_InputArena, *this, *pCVars_);
    auto mouse = core::makeUnique<XMouse>(g_InputArena, *this, *pCVars_);

    // weak refs
    pKeyBoard_ = keyBoard.ptr();
    pMouse_ = mouse.ptr();

    // o baby!
    if (!addInputDevice(XInputDevicePtr(std::move(keyBoard)))) {
        X_ERROR("Input", "Failed to add keyboard input device");
        return false;
    }

    if (!addInputDevice(XInputDevicePtr(std::move(mouse)))) {
        X_ERROR("Input", "Failed to add mouse input device");
        return false;
    }

    clearKeyState();
    return true;
}

void XWinInput::shutDown(void)
{
    XBaseInput::shutDown();
}

void XWinInput::release(void)
{
    X_DELETE(this, g_InputArena);
}

void XWinInput::update(core::FrameInput& inputFrame)
{
    X_PROFILE_BEGIN("Win32RawInput", core::profiler::SubSys::INPUT);

    inputFrame.cusorPos = core::xWindow::GetCusroPos();
    auto* pWindow = gEnv->pCore->GetGameWindow();
    if (pWindow) {
        inputFrame.cusorPosClient = pWindow->GetCusroPosClient();
    }

    XBaseInput::update(inputFrame);

    RAWINPUT X_ALIGNED_SYMBOL(input[BUF_NUM], 8);

    UINT size;
    size_t num;

    const bool debug = (pCVars_->inputDebug_ > 1);
    
    // this only returns results for windows created on the same thread.
    X_ASSERT(gEnv->mainThreadId == core::Thread::getCurrentID(), "Input must be got from main thread")();

    for (;;) {
        size = sizeof(input);
        num = GetRawInputBuffer(input, &size, static_cast<UINT>(ENTRY_HDR_SIZE));

        // log size even if empty
        if (debug) {
            X_LOG0("Input", "Buffer size: %i threadId: 0x%x", num, core::Thread::getCurrentID());
        }

        if (num == 0) {
            break;
        }

        if (num == static_cast<UINT>(-1)) {
            core::lastError::Description Dsc;
            X_ERROR("Input", "Failed to get input. Error: %s", core::lastError::ToString(Dsc));
            break;
        }

        const size_t eventSpace = inputFrame.events.capacity() - inputFrame.events.size();
        if (eventSpace < num) {
            X_WARNING("Input", "Input frame buffer full ignoring %i events", num - eventSpace);
            num = eventSpace;
        }

        PRAWINPUT rawInput = input;
        for (UINT i = 0; i < num; ++i) {
            const uint8_t* pData = reinterpret_cast<const uint8_t*>(&rawInput->data);
            // needs to be 16 + 8 aligned.
            if (isWow64_) {
                pData += 8;
            }

            if (rawInput->header.dwType == RIM_TYPEMOUSE) {
                pMouse_->processInput(pData, inputFrame);
            }
            else if (rawInput->header.dwType == RIM_TYPEKEYBOARD) {
                pKeyBoard_->processInput(pData, inputFrame);
            }

            rawInput = NEXTRAWINPUTBLOCK(rawInput);
        }
    }
}

void XWinInput::clearKeyState(void)
{
    XBaseInput::clearKeyState();
}

X_NAMESPACE_END