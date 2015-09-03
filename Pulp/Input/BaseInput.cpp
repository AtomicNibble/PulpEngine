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
	Devices_(g_InputArena),
	holdSymbols_(g_InputArena)
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

	for (TInputDevices::Iterator i = Devices_.begin(); i != Devices_.end(); ++i)
		(*i)->ShutDown();

	std::for_each(Devices_.begin(), Devices_.end(), delete_ptr());
	Devices_.clear();


	if (!Listners_.empty())
	{
		X_WARNING("InputSys", "%i listners still registered", Listners_.size());
	}
	if (!consoleListeners_.empty())
	{
		X_WARNING("InputSys", "%i console listners still registered", consoleListeners_.size());
	}

	Listners_.clear();
	consoleListeners_.clear();
}


void XBaseInput::release(void)
{
	X_DELETE(this,g_InputArena);
}

void XBaseInput::PostInit()
{
	for (TInputDevices::Iterator i = Devices_.begin(); i != Devices_.end(); ++i)
	{
		(*i)->PostInit();
	}
}

void XBaseInput::Update(bool bFocus)
{
	hasFocus_ = bFocus;

	PostHoldEvents();

	for (TInputDevices::Iterator i = Devices_.begin(); i != Devices_.end(); ++i)
	{
		if ((*i)->IsEnabled()) {
			(*i)->Update(bFocus);
		}
	}

	// send commit event after all input processing for this frame has finished
	InputEvent event;
	event.modifiers = modifiers_;
	event.deviceId = InputDevice::UNKNOWN;
	event.value = 0;
	event.name = "final";
	event.keyId = KeyId::UNKNOWN;
	PostInputEvent(event);
}




void XBaseInput::ClearKeyState()
{
	if (g_pInputCVars->input_debug)
	{
		X_LOG0("Input", "clearing key states.");
	}

	modifiers_.Clear();

	for (TInputDevices::Iterator i = Devices_.begin(); i != Devices_.end(); ++i)
	{
		(*i)->ClearKeyState();
	}

	retriggering_ = false;
	holdSymbols_.clear();
}


void XBaseInput::RetriggerKeyState()
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
	if (std::find(Listners_.begin(), Listners_.end(), pListener) == Listners_.end())
	{
		Listners_.push_back(pListener);
		Listners_.sort(compareInputListener);
	}
}

void XBaseInput::RemoveEventListener(IInputEventListner *pListener)
{
	// Remove listener if it is in list.
	TInputEventListeners::iterator it = std::find(Listners_.begin(), Listners_.end(), pListener);
	if (it != Listners_.end())
	{
		Listners_.erase(it);
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
	if (it != consoleListeners_.end()) consoleListeners_.erase(it);
}



bool XBaseInput::AddInputDevice(IInputDevice* pDevice)
{
	if (pDevice)
	{
		if (pDevice->Init())
		{
			Devices_.push_back(pDevice);
			return true;
		}
		X_DELETE( pDevice, g_InputArena);
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

bool XBaseInput::IsEventPostingEnabled() const
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

void XBaseInput::EnableDevice(InputDevice::Enum deviceId, bool enable)
{
	if (deviceId < (int)Devices_.size())
	{
		// try to flush the device ... perform a dry update
		EnableEventPosting(false);
		Devices_[deviceId]->Update(hasFocus_);
		EnableEventPosting(true);
		Devices_[deviceId]->Enable(enable);
	}
}


bool XBaseInput::HasInputDeviceOfType(InputDeviceType::Enum type) const
{
	for (TInputDevices::ConstIterator i = Devices_.begin(); i != Devices_.end(); ++i)
	{
		if ((*i)->IsOfDeviceType(type))
			return true;
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
void XBaseInput::PostHoldEvents()
{
	X_PROFILE_BEGIN("PostHoldEvents", core::ProfileSubSys::INPUT);

	InputEvent event;
	const size_t count = holdSymbols_.size();

	if (g_pInputCVars->input_debug > 2)
	{ 
		X_LOG0("Input", "posting %i hold symbols", count);
	}

	for (size_t i = 0; i < count; ++i)
	{
		holdSymbols_[i]->AssignToEvent(event, GetModifiers());
		PostInputEvent(event);
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
		for (TInputEventListeners::const_iterator it = Listners_.begin(); it != Listners_.end(); ++it)
		{
		//	assert(*it);
			if (*it == NULL)
				continue;

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
	size_t slot = std::numeric_limits<size_t>::max();
	size_t last = holdSymbols_.size() - 1;

	for (size_t i = last; i >= 0; --i)
	{
		if (holdSymbols_[i] == pSymbol)
		{
			slot = i;
			break;
		}
	}
	if (slot != std::numeric_limits<size_t>::max())
	{
		// swap last and found symbol
		holdSymbols_[slot] = holdSymbols_[last];
		// pop last ... which is now the one we want to get rid of
		holdSymbols_.pop_back();
	}
}

XBaseInput::ModifierFlags XBaseInput::GetModifiers()
{
	return modifiers_;
}

void XBaseInput::SetModifiers(ModifierFlags flags)
{
	this->modifiers_ = flags;
}

void XBaseInput::ClearModifiers()
{
	this->modifiers_.Clear();
}

X_NAMESPACE_END