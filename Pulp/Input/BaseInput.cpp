#include "stdafx.h"
#include "BaseInput.h"
#include "InputCVars.h"

#include <algorithm>

X_NAMESPACE_BEGIN(input)

namespace
{
    bool compareInputListener(const IInputEventListner* pListenerA, const IInputEventListner* pListenerB)
    {
        X_ASSERT_NOT_NULL(pListenerA);
        X_ASSERT_NOT_NULL(pListenerB);

        if (pListenerA->GetInputPriority() > pListenerB->GetInputPriority()) {
            return true;
        }

        return false;
    }

} // namespace

XBaseInput::XBaseInput(core::MemoryArenaBase* arena) :
    arena_(arena),
    pCVars_(X_NEW(XInputCVars, arena, "InputCvars")),
    devices_(arena),
    listners_(arena),
    consoleListeners_(arena),
    holdSymbols_(arena),
    clearStateEvents_(arena),
    enableEventPosting_(true),
    hasFocus_(false),
    retriggering_(false)
{
    holdSymbols_.setGranularity(32);
    holdSymbols_.reserve(64);
    clearStateEvents_.reserve(16);
    clearStateEvents_.setGranularity(16);
}

XBaseInput::~XBaseInput(void)
{
    X_DELETE(pCVars_, arena_);
}

void XBaseInput::registerVars(void)
{
    pCVars_->registerVars();
}

void XBaseInput::registerCmds(void)
{
}

bool XBaseInput::Init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pCore);

    // register event listner.
    gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);

    return true;
}

void XBaseInput::ShutDown(void)
{
    X_LOG0("InputSys", "Shutting Down");

    gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);

    for (auto& device : devices_) {
        device->ShutDown();
    }

    std::for_each(devices_.begin(), devices_.end(), [](IInputDevice* pDevice) { X_DELETE(pDevice, g_InputArena); });
    devices_.clear();

    if (listners_.isNotEmpty()) {
        X_WARNING("InputSys", "%i listners still registered", listners_.size());
    }
    if (consoleListeners_.isNotEmpty()) {
        X_WARNING("InputSys", "%i console listners still registered", consoleListeners_.size());
    }

    listners_.clear();
    consoleListeners_.clear();
}

void XBaseInput::release(void)
{
    X_DELETE(this, g_InputArena);
}

void XBaseInput::PostInit(void)
{
    for (auto& device : devices_) {
        device->PostInit();
    }
}

void XBaseInput::Update(core::FrameData& frameData)
{
    hasFocus_ = frameData.flags.IsSet(core::FrameFlag::HAS_FOCUS);

    AddClearEvents(frameData.input);
    AddHoldEvents(frameData.input);

    for (auto& device : devices_) {
        if (device->IsEnabled()) {
            device->Update(frameData);
        }
    }
}

void XBaseInput::ClearKeyState(void)
{
    if (pCVars_->inputDebug_) {
        X_LOG0("Input", "clearing key states.");
    }

    // clear the modifiers, devices will restore current modifiers values that are set, like capslock.
    modifiers_.Clear();

    // when we clear states some devices might want to broadcast release events.
    // we store them to be picked up by the next frame.
    for (auto& device : devices_) {
        device->ClearKeyState(clearStateEvents_);
    }

    retriggering_ = false;
    holdSymbols_.clear();
}

void XBaseInput::RetriggerKeyState(void)
{
    retriggering_ = true;
    InputEvent event;

    const size_t count = holdSymbols_.size();
    for (size_t i = 0; i < count; ++i) {
        InputState::Enum oldState = holdSymbols_[i]->state;
        holdSymbols_[i]->state = InputState::PRESSED;
        holdSymbols_[i]->AssignToEvent(event, GetModifiers());
        PostInputEvent(event);
        holdSymbols_[i]->state = oldState;
    }
    retriggering_ = false;
}

void XBaseInput::AddEventListener(IInputEventListner* pListener)
{
    // Add new listener to list if not added yet.
    if (std::find(listners_.begin(), listners_.end(), pListener) == listners_.end()) {
        listners_.push_back(pListener);
        std::stable_sort(listners_.begin(), listners_.end(), compareInputListener);
    }
}

void XBaseInput::RemoveEventListener(IInputEventListner* pListener)
{
    // Remove listener if it is in list.
    auto it = std::find(listners_.begin(), listners_.end(), pListener);
    if (it != listners_.end()) {
        listners_.erase(it);
    }
}

void XBaseInput::AddConsoleEventListener(IInputEventListner* pListener)
{
    if (std::find(consoleListeners_.begin(), consoleListeners_.end(), pListener) == consoleListeners_.end()) {
        consoleListeners_.push_back(pListener);
        std::stable_sort(consoleListeners_.begin(), consoleListeners_.end(), compareInputListener);
    }
}

void XBaseInput::RemoveConsoleEventListener(IInputEventListner* pListener)
{
    auto it = std::find(consoleListeners_.begin(), consoleListeners_.end(), pListener);
    if (it != consoleListeners_.end()) {
        consoleListeners_.erase(it);
    }
}

bool XBaseInput::AddInputDevice(IInputDevice* pDevice)
{
    if (pDevice) {
        if (pDevice->Init()) {
            devices_.push_back(pDevice);
            return true;
        }
        X_DELETE(pDevice, g_InputArena);
    }
    return false;
}

void XBaseInput::EnableEventPosting(bool bEnable)
{
    enableEventPosting_ = bEnable;
    if (pCVars_->inputDebug_) {
        X_LOG0("Input", "Eventposting: %s", bEnable ? "true" : "false");
    }
}

bool XBaseInput::PostInputEvent(const InputEvent& event)
{
    if (event.keyId == KeyId::UNKNOWN) {
        if (pCVars_->inputDebug_ > 1) {
            X_WARNING("Input", "Ingoring unknown event key from device: %s", InputDeviceType::ToString(event.deviceType));
        }
        return false;
    }

    if (!SendEventToListeners(event)) {
        return false;
    }

    AddEventToHoldSymbols(event);
    return true;
}

bool XBaseInput::Job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData)
{
    X_UNUSED(jobSys);

    if (!enableEventPosting_) {
        X_LOG2("Input", "Input posting is disable");
        return false;
    }

    const auto& input = frameData.input;
    for (const auto& e : input.events) {
        PostInputEvent(e);
    }

    return true;
}

void XBaseInput::EnableDevice(InputDeviceType::Enum deviceType, bool enable)
{
    for (auto& device : devices_) {
        if (device->IsOfDeviceType(deviceType)) {
            device->Enable(enable);
            break;
        }
    }
}

bool XBaseInput::HasInputDeviceOfType(InputDeviceType::Enum type) const
{
    for (const auto& device : devices_) {
        if (device->IsOfDeviceType(type)) {
            return true;
        }
    }
    return false;
}

void XBaseInput::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
    X_UNUSED(wparam, lparam);

    if (event == CoreEvent::CHANGE_FOCUS || event == CoreEvent::LEVEL_LOAD_START || event == CoreEvent::LEVEL_POST_UNLOAD) {
        ClearKeyState();
    }
}

void XBaseInput::AddClearEvents(core::FrameInput& inputFrame)
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
        InputEvent& event = inputFrame.events.AddOne();
        event = clearStateEvents_[i];
    }

    clearStateEvents_.clear();
}

// Hold symbols shizzz
void XBaseInput::AddHoldEvents(core::FrameInput& inputFrame)
{
    X_PROFILE_BEGIN("PostHoldEvents", core::profiler::SubSys::INPUT);

    size_t count = holdSymbols_.size();

    if (pCVars_->inputDebug_ > 2) {
        X_LOG0("Input", "posting %i hold symbols", count);
    }

    const size_t eventSpace = inputFrame.events.capacity() - inputFrame.events.size();
    if (eventSpace < count) {
        X_WARNING("Input", "Input frame buffer full ignoring %i hold events", count - eventSpace);
        count = eventSpace;
    }

    ModifierFlags modifiers = GetModifiers();
    for (size_t i = 0; i < count; ++i) {
        InputEvent& event = inputFrame.events.AddOne();
        holdSymbols_[i]->AssignToEvent(event, modifiers);
    }
}

bool XBaseInput::SendEventToListeners(const InputEvent& event)
{
    // return true if add to hold.

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

    return true;
}

void XBaseInput::AddEventToHoldSymbols(const InputEvent& event)
{
    if (!retriggering_ && event.pSymbol) {
        if (event.pSymbol->state == InputState::PRESSED) {
            event.pSymbol->state = InputState::DOWN;
            holdSymbols_.push_back(event.pSymbol);
        }
        else if (event.pSymbol->state == InputState::RELEASED && !holdSymbols_.isEmpty()) {
            ClearHoldEvent(event.pSymbol);
        }
    }
}

void XBaseInput::ClearHoldEvent(InputSymbol* pSymbol)
{
    // remove hold key
    int32_t slot = std::numeric_limits<int32_t>::max();
    int32_t last = safe_static_cast<int32_t, size_t>(holdSymbols_.size() - 1);

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

X_NAMESPACE_END