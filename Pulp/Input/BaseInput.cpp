#include "stdafx.h"
#include "BaseInput.h"
#include "InputCVars.h"

#include "InputDevice.h"

#include <algorithm>

X_NAMESPACE_BEGIN(input)


XBaseInput::XBaseInput(core::MemoryArenaBase* arena) :
    arena_(arena),
    pCVars_(core::makeUnique<XInputCVars>(arena)),
    devices_(arena),
    holdSymbols_(arena),
    clearStateEvents_(arena)
{
    holdSymbols_.setGranularity(32);
    holdSymbols_.reserve(64);
    clearStateEvents_.reserve(16);
}

XBaseInput::~XBaseInput(void)
{

}

void XBaseInput::registerVars(void)
{
    pCVars_->registerVars();
}

void XBaseInput::registerCmds(void)
{
}

bool XBaseInput::init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);

    // register event listner.
    gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);

    return true;
}

void XBaseInput::shutDown(void)
{
    X_LOG0("InputSys", "Shutting Down");

    gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);

    for (auto& pDevice : devices_) {
        pDevice->shutDown();
    }

    devices_.clear();
}

void XBaseInput::release(void)
{
    X_DELETE(this, g_InputArena);
}

bool XBaseInput::job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData)
{
    X_UNUSED(jobSys);

    const auto& input = frameData.input;
    for (const auto& e : input.events) {
        postInputEvent(e);
    }

    return true;
}

void XBaseInput::update(core::FrameData& frameData)
{
    addClearEvents(frameData.input);
    addHoldEvents(frameData.input);
}

void XBaseInput::clearKeyState(void)
{
    if (pCVars_->inputDebug_) {
        X_LOG0("Input", "clearing key states.");
    }

    // clear the modifiers, devices will restore current modifiers values that are set, like capslock.
    modifiers_.Clear();

    // when we clear states some devices might want to broadcast release events.
    // we store them to be picked up by the next frame.
    for (auto& pDevice : devices_) {
        pDevice->clearKeyState(clearStateEvents_);
    }

    holdSymbols_.clear();
}

bool XBaseInput::addInputDevice(XInputDevicePtr pDevice)
{
    if (!pDevice) {
        return false;
    }

    if (!pDevice->init(*this)) {
        return false;
    }
    
    devices_.emplace_back(std::move(pDevice));
    return true;
}

void XBaseInput::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
    X_UNUSED(wparam, lparam);

    if (event == CoreEvent::CHANGE_FOCUS || event == CoreEvent::LEVEL_LOAD_START || event == CoreEvent::LEVEL_POST_UNLOAD) {
        clearKeyState();
    }
}

void XBaseInput::addClearEvents(core::FrameInput& inputFrame)
{
    if (clearStateEvents_.isEmpty()) {
        return;
    }

    size_t num = clearStateEvents_.size();
    if (pCVars_->inputDebug_ > 0) {
        X_LOG0("Input", "posting %i clear events", num);
    }

    const size_t eventSpace = inputFrame.events.capacity() - inputFrame.events.size();
    if (eventSpace < num) {
        // this is bad, since clear events are important.
        X_ERROR("Input", "Input frame buffer full ignoring %i clear events", num - eventSpace);
        num = eventSpace;
    }

    for (size_t i = 0; i < num; ++i) {
        inputFrame.events.emplace_back(clearStateEvents_[i]);
    }

    if (eventSpace < num) {
        clearStateEvents_.erase(clearStateEvents_.begin(), clearStateEvents_.begin() + num);
    }
    else {
        clearStateEvents_.clear();
    }
}

// Hold symbols shizzz
void XBaseInput::addHoldEvents(core::FrameInput& inputFrame)
{
    size_t count = holdSymbols_.size();

    if (pCVars_->inputDebug_ > 2) {
        X_LOG0("Input", "posting %i hold symbols", count);
    }

    const size_t eventSpace = inputFrame.events.capacity() - inputFrame.events.size();
    if (eventSpace < count) {
        X_WARNING("Input", "Input frame buffer full ignoring %i hold events", count - eventSpace);
        count = eventSpace;
    }

    ModifierFlags modifiers = getModifiers();
    for (size_t i = 0; i < count; ++i) {
        InputEvent& event = inputFrame.events.AddOne();
        holdSymbols_[i]->AssignToEvent(event, modifiers);
    }
}

void XBaseInput::addEventToHoldSymbols(const InputEvent& event)
{
    if (!event.pSymbol) {
        return;
    }

    if (event.pSymbol->state == InputState::PRESSED) {
        event.pSymbol->state = InputState::DOWN;
        holdSymbols_.push_back(event.pSymbol);
    }
    else if (event.pSymbol->state == InputState::RELEASED && !holdSymbols_.isEmpty()) {
        clearHoldEvent(event.pSymbol);
    }
}

void XBaseInput::clearHoldEvent(const InputSymbol* pSymbol)
{
    // remove hold key
    int32_t slot = std::numeric_limits<int32_t>::max();
    int32_t last = safe_static_cast<int32_t>(holdSymbols_.size() - 1);

    for (int32_t i = last; i >= 0; --i) {
        if (holdSymbols_[i] == pSymbol) {
            slot = i;
            break;
        }
    }
    if (slot != std::numeric_limits<int32_t>::max()) {
        // swap last and found symbol
        holdSymbols_[slot] = holdSymbols_[last];
        // pop last ... which is now the one we want to get rid of
        holdSymbols_.pop_back();
    }
}

bool XBaseInput::postInputEvent(const InputEvent& event)
{
    if (event.keyId == KeyId::UNKNOWN) {
        if (pCVars_->inputDebug_ > 1) {
            X_WARNING("Input", "Ingoring unknown event key from device: %s", InputDeviceType::ToString(event.deviceType));
        }
        return false;
    }

    if (!sendEventToListeners(event)) {
        return false;
    }

    addEventToHoldSymbols(event);
    return true;
}

bool XBaseInput::sendEventToListeners(const InputEvent& event)
{
    // return true if add to hold.
#if 1
    X_UNUSED(event);
#else
    if (event.action == InputState::CHAR) {
        for (auto* pListener : consoleListeners_) {
            if (pListener->OnInputEventChar(event)) {
                return false;
            }
        }
        for (auto* pListener : listners_) {
            if (pListener->OnInputEventChar(event)) {
                break;
            }
        }
    }
    else {
        for (auto* pListener : consoleListeners_)

        {
            if (pListener->OnInputEvent(event)) {
                return false;
            }
        }
        for (auto* pListener : listners_) {
            if (pListener->OnInputEvent(event)) {
                break;
            }
        }
    }
#endif
    return true;
}


X_NAMESPACE_END