#include "stdafx.h"
#include "BaseInput.h"
#include "InputCVars.h"

#include <algorithm>

X_NAMESPACE_BEGIN(input)

namespace {

	bool compareInputListener(const IInputEventListner* pListenerA, const IInputEventListner* pListenerB)
	{
		X_ASSERT_NOT_NULL(pListenerA);
		X_ASSERT_NOT_NULL(pListenerB);

		if (!pListenerA || !pListenerB)
			return false;

		if (pListenerA->GetPriority() > pListenerB->GetPriority())
		{
			return true;
		}

		return false;
	}

	struct delete_ptr { // Helper function to ease cleanup of container
		template <typename P>
		void operator () (P p) {
			X_DELETE(p, g_InputArena);
		}
	};
}

XBaseInput::XBaseInput() :
	pCVars_(X_NEW(XInputCVars,g_InputArena,"InputCvars")),
	devices_(g_InputArena),
	holdSymbols_(g_InputArena),
	enableEventPosting_(true),
	hasFocus_(false),
	retriggering_(false)
{
	g_pInputCVars = pCVars_;

	holdSymbols_.reserve(64);
}


XBaseInput::~XBaseInput()
{
	X_DELETE(pCVars_, g_InputArena);
	pCVars_ = nullptr;
	g_pInputCVars = nullptr;
}


bool XBaseInput::Init()
{
	// register even listner.
	gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);
	
	return true;
}

void XBaseInput::ShutDown()
{
	X_LOG0("InputSys", "Shutting Down");

	gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);

	for (TInputDevices::Iterator it = devices_.begin(); it != devices_.end(); ++it) {
		(*it)->ShutDown();
	}

	std::for_each(devices_.begin(), devices_.end(), delete_ptr());
	devices_.clear();


	if (!listners_.empty())
	{
		X_WARNING("InputSys", "%i listners still registered", listners_.size());
	}
	if (!consoleListeners_.empty())
	{
		X_WARNING("InputSys", "%i console listners still registered", consoleListeners_.size());
	}

	listners_.clear();
	consoleListeners_.clear();
}


void XBaseInput::release(void)
{
	X_DELETE(this,g_InputArena);
}

void XBaseInput::PostInit(void)
{
	for (TInputDevices::Iterator it = devices_.begin(); it != devices_.end(); ++it)
	{
		(*it)->PostInit();
	}
}

void XBaseInput::Update(core::V2::Job* pInputJob, core::FrameData& frameData)
{
	X_UNUSED(pInputJob);

	hasFocus_ = frameData.flags.IsSet(core::FrameFlag::HAS_FOCUS);

	AddHoldEvents(frameData.input);

	for (TInputDevices::Iterator it = devices_.begin(); it != devices_.end(); ++it)
	{
		if ((*it)->IsEnabled()) {
			(*it)->Update(frameData);
		}
	}
}




void XBaseInput::ClearKeyState(void)
{
	if (g_pInputCVars->input_debug)
	{
		X_LOG0("Input", "clearing key states.");
	}

	modifiers_.Clear();

	for (TInputDevices::Iterator it = devices_.begin(); it != devices_.end(); ++it)
	{
		(*it)->ClearKeyState();
	}

	retriggering_ = false;
	holdSymbols_.clear();
}


void XBaseInput::RetriggerKeyState(void)
{
	retriggering_ = true;
	InputEvent event;

	const size_t count = holdSymbols_.size();
	for (size_t i = 0; i < count; ++i)
	{
		InputState::Enum oldState = holdSymbols_[i]->state;
		holdSymbols_[i]->state = InputState::PRESSED;
		holdSymbols_[i]->AssignToEvent(event, GetModifiers());
		PostInputEvent(event);
		holdSymbols_[i]->state = oldState;
	}
	retriggering_ = false;

}

void XBaseInput::AddEventListener(IInputEventListner *pListener)
{
	// Add new listener to list if not added yet.
	if (std::find(listners_.begin(), listners_.end(), pListener) == listners_.end())
	{
		listners_.push_back(pListener);
		listners_.sort(compareInputListener);
	}
}

void XBaseInput::RemoveEventListener(IInputEventListner *pListener)
{
	// Remove listener if it is in list.
	TInputEventListeners::iterator it = std::find(listners_.begin(), listners_.end(), pListener);
	if (it != listners_.end())
	{
		listners_.erase(it);
	}
}

void XBaseInput::AddConsoleEventListener(IInputEventListner *pListener)
{
	if (std::find(consoleListeners_.begin(), consoleListeners_.end(), pListener) == consoleListeners_.end())
	{
		consoleListeners_.push_back(pListener);
		consoleListeners_.sort(compareInputListener);
	}
}

void XBaseInput::RemoveConsoleEventListener(IInputEventListner *pListener)
{
	TInputEventListeners::iterator it = std::find(consoleListeners_.begin(), consoleListeners_.end(), pListener);
	if (it != consoleListeners_.end()) {
		consoleListeners_.erase(it);
	}
}



bool XBaseInput::AddInputDevice(IInputDevice* pDevice)
{
	if (pDevice)
	{
		if (pDevice->Init())
		{
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
	if (g_pInputCVars->input_debug)
	{
		X_LOG0("Input", "Eventposting: %s", bEnable ? "true" : "false");
	}
}

bool XBaseInput::IsEventPostingEnabled(void) const
{
	return enableEventPosting_;
}

bool XBaseInput::PostInputEvent(const InputEvent &event, bool bForce)
{
	if (!bForce && !enableEventPosting_)
	{
		return false;
	}

	if (event.keyId == KeyId::UNKNOWN)
	{
		return false;
	}

	if (!SendEventToListeners(event))
		return false;

	AddEventToHoldSymbols(event);
	return true;
}

void XBaseInput::EnableDevice(InputDeviceType::Enum deviceType, bool enable)
{
	for (TInputDevices::ConstIterator it = devices_.begin(); it != devices_.end(); ++it)
	{
		if ((*it)->IsOfDeviceType(deviceType))
		{
			IInputDevice* pDevice = (*it);
			pDevice->Enable(enable);
			break;
		}
	}
}


bool XBaseInput::HasInputDeviceOfType(InputDeviceType::Enum type) const
{
	for (TInputDevices::ConstIterator i = devices_.begin(); i != devices_.end(); ++i)
	{
		if ((*i)->IsOfDeviceType(type)) {
			return true;
		}
	}
	return false;
}

void XBaseInput::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(wparam);
	X_UNUSED(lparam);
	if (
		event == CoreEvent::CHANGE_FOCUS ||
		event == CoreEvent::LEVEL_LOAD_START ||
		event == CoreEvent::LEVEL_POST_UNLOAD)
	{
		ClearKeyState();
	}
}


// Hold symbols shizzz
void XBaseInput::AddHoldEvents(core::FrameInput& inputFrame)
{
	X_PROFILE_BEGIN("PostHoldEvents", core::ProfileSubSys::INPUT);

	size_t count = holdSymbols_.size();

	if (g_pInputCVars->input_debug > 2)
	{ 
		X_LOG0("Input", "posting %i hold symbols", count);
	}

	const size_t eventSpace = inputFrame.events.capacity() - inputFrame.events.size();
	if (eventSpace < count) {
		X_WARNING("Input", "Input frame buffer full ignoring %i hold events", count - eventSpace);
		count = eventSpace;
	}

	for (size_t i = 0; i < count; ++i)
	{
		InputEvent event = inputFrame.events.AddOne();
		holdSymbols_[i]->AssignToEvent(event, GetModifiers());
	}
}

bool XBaseInput::SendEventToListeners(const InputEvent &event)
{
	// console listeners get to process the event first
	for (TInputEventListeners::const_iterator it = consoleListeners_.begin(); it != consoleListeners_.end(); ++it)
	{
		bool ret = false;
		if (event.action != InputState::CHAR)
			ret = (*it)->OnInputEvent(event);
		else
			ret = (*it)->OnInputEventChar(event);

		if (ret)
			return false;
	}


	// Don't post events if input is being blocked
	// but still need to update held inputs
	bool bInputBlocked = false; // ShouldBlockInputEventPosting(event.keyId, event.deviceId);
	if (!bInputBlocked)
	{
		// Send this event to all listeners until the first one returns true.
		for (TInputEventListeners::const_iterator it = listners_.begin(); it != listners_.end(); ++it)
		{
			bool ret = false;
			if (event.action != InputState::CHAR)
				ret = (*it)->OnInputEvent(event);
			else
				ret = (*it)->OnInputEventChar(event);

			if (ret) {
				break;
			}
		}
	}

	return true;
}

void XBaseInput::AddEventToHoldSymbols(const InputEvent& event)
{
#if 1
	if (!retriggering_ && event.pSymbol)
	{
		if (event.pSymbol->state == InputState::PRESSED)
		{
			event.pSymbol->state = InputState::DOWN;
			holdSymbols_.push_back(event.pSymbol);
		}
		else if (event.pSymbol->state == InputState::RELEASED && !holdSymbols_.isEmpty())
		{
			ClearHoldEvent(event.pSymbol);
		}
	}
#endif
}


void XBaseInput::ClearHoldEvent(InputSymbol* pSymbol)
{
	// remove hold key
	int32_t slot = std::numeric_limits<int32_t>::max();
	int32_t last = safe_static_cast<int32_t,size_t>(holdSymbols_.size() - 1);

	for (int32_t i = last; i >= 0; --i)
	{
		if (holdSymbols_[i] == pSymbol)
		{
			slot = i;
			break;
		}
	}
	if (slot != std::numeric_limits<int32_t>::max())
	{
		// swap last and found symbol
		holdSymbols_[slot] = holdSymbols_[last];
		// pop last ... which is now the one we want to get rid of
		holdSymbols_.pop_back();
	}
}

XBaseInput::ModifierFlags XBaseInput::GetModifiers(void)
{
	return modifiers_;
}

void XBaseInput::SetModifiers(ModifierFlags flags)
{
	this->modifiers_ = flags;
}

void XBaseInput::ClearModifiers(void)
{
	this->modifiers_.Clear();
}

X_NAMESPACE_END