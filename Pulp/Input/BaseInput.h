#pragma once

#ifndef _X_INPUT_BASE_H_
#define _X_INPUT_BASE_H_

#include <ICore.h>
#include <IFrameData.h>

#include <Containers\Array.h>


#include <list>
// #include <vector>

X_NAMESPACE_BEGIN(input)

struct IInputEventListner;
class XInputCVars;

class XBaseInput : public IInput, public ICoreEventListener
{
public:
	XBaseInput();
	virtual ~XBaseInput() X_OVERRIDE;

	// IInput
	// stub implementation
	bool Init(void) X_OVERRIDE;
	void PostInit(void) X_OVERRIDE;
	void Update(core::V2::Job* pInputJob, core::FrameData& frameData) X_OVERRIDE;
	void ShutDown(void) X_OVERRIDE;
	void release(void) X_OVERRIDE;
		 
	void ClearKeyState(void) X_OVERRIDE;
	void RetriggerKeyState(void) X_OVERRIDE;
	X_INLINE bool	Retriggering(void) const X_OVERRIDE;
	bool HasInputDeviceOfType(InputDeviceType::Enum type) const X_OVERRIDE;
	bool AddInputDevice(IInputDevice* pDevice) X_OVERRIDE;
	void EnableEventPosting(bool bEnable)X_OVERRIDE;
	bool IsEventPostingEnabled() const X_OVERRIDE;
	bool PostInputEvent(const InputEvent &event, bool bForce = false);

	// listener functions (implemented)
	void AddEventListener(IInputEventListner* pListener) X_OVERRIDE;
	void RemoveEventListener(IInputEventListner* pListener) X_OVERRIDE;
	void AddConsoleEventListener(IInputEventListner* pListener) X_OVERRIDE;
	void RemoveConsoleEventListener(IInputEventListner* pLstener) X_OVERRIDE;


	void EnableDevice(InputDeviceType::Enum deviceType, bool enable) X_OVERRIDE;
	// ~IInput

	// ISystemEventListener
	void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ISystemEventListener

	X_INLINE bool HasFocus(void) const;

	ModifierFlags GetModifiers(void) X_OVERRIDE;
	void SetModifiers(ModifierFlags flags) X_OVERRIDE;
	void ClearModifiers(void);

	X_INLINE InputSymbol* DefineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
		const KeyName& name_, InputSymbol::Type type_ = InputSymbol::Type::Button,
		ModifiersMasks::Enum mod_mask = ModifiersMasks::NONE) X_OVERRIDE;

protected:
	void PostHoldEvents(void);

private:

	void ClearHoldEvent(InputSymbol* pSymbol);
	bool SendEventToListeners(const InputEvent &event);
	void AddEventToHoldSymbols(const InputEvent &event);

//	void RemoveDeviceHoldSymbols(EDeviceId deviceId, uint8 deviceIndex);

protected:
	// listener functionality
	typedef core::Array<IInputDevice*>		TInputDevices;
	typedef std::list<IInputEventListner*>	TInputEventListeners;
	typedef core::Array<InputSymbol*>		TInputSymbols;

	TInputSymbols						holdSymbols_;
	TInputEventListeners				Listners_;
	TInputEventListeners				consoleListeners_;

	// input device management
	TInputDevices						devices_;

	// CVars
	XInputCVars*						pCVars_;

	bool								enableEventPosting_;
	bool								retriggering_;

	bool								hasFocus_;

	ModifierFlags						modifiers_;	// caps ALT, SHIFT etc.

	InputSymbol InputSymbols_[input::KeyId::MOUSE_LAST];

private:
	X_NO_ASSIGN(XBaseInput);
	X_NO_COPY(XBaseInput);
};


X_NAMESPACE_END

#include "BaseInput.inl"

#endif // !_X_INPUT_BASE_H_
