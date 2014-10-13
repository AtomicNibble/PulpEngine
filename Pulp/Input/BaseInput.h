#pragma once

#ifndef _X_INPUT_BASE_H_
#define _X_INPUT_BASE_H_

#include <ICore.h>

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
	virtual ~XBaseInput();

	// IInput
	// stub implementation
	virtual bool	Init(void) X_OVERRIDE;
	virtual void	PostInit(void) X_OVERRIDE;
	virtual void	Update(bool bFocus) X_OVERRIDE;
	virtual void	ShutDown(void) X_OVERRIDE;
	virtual void	release(void) X_OVERRIDE;

	virtual void	ClearKeyState() X_OVERRIDE;
	virtual void	RetriggerKeyState() X_OVERRIDE;
	virtual bool	Retriggering()	X_OVERRIDE{ return retriggering_; }
	virtual bool	HasInputDeviceOfType(InputDeviceType::Enum type) const X_OVERRIDE;
	virtual bool    AddInputDevice(IInputDevice* pDevice) X_OVERRIDE;
	virtual void	EnableEventPosting(bool bEnable)X_OVERRIDE;
	virtual bool    IsEventPostingEnabled() const X_OVERRIDE;
	virtual bool	PostInputEvent(const InputEvent &event, bool bForce = false);

	// listener functions (implemented)
	virtual void	AddEventListener(IInputEventListner* pListener) X_OVERRIDE;
	virtual void	RemoveEventListener(IInputEventListner* pListener) X_OVERRIDE;
	virtual void	AddConsoleEventListener(IInputEventListner* pListener) X_OVERRIDE;
	virtual void	RemoveConsoleEventListener(IInputEventListner* pLstener) X_OVERRIDE;


	virtual void	EnableDevice(InputDevice::Enum deviceId, bool enable) X_OVERRIDE;
	// ~IInput

	// ISystemEventListener
	virtual void	OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ISystemEventListener

	bool	HasFocus() const		{ return hasFocus_; }

	virtual ModifierFlags GetModifiers();
	virtual void SetModifiers(ModifierFlags flags);
	void ClearModifiers();

	virtual InputSymbol* DefineSymbol(InputDevice::Enum device, KeyId::Enum id_, 
		const KeyName& name_, InputSymbol::Type type_ = InputSymbol::Type::Button, 
		ModifiersMasks::Enum  mod_mask = ModifiersMasks::NONE) X_OVERRIDE {

		InputSymbol* pSymbol = &InputSymbols_[id_];

		pSymbol->deviceId = device;
		pSymbol->name = name_;
		pSymbol->type = type_;
		pSymbol->modifer_mask = mod_mask;
		pSymbol->keyId = id_;

		return pSymbol;
	}

protected:

	void	PostHoldEvents();

private:

	void	ClearHoldEvent(InputSymbol* pSymbol);
	bool	SendEventToListeners(const InputEvent &event);
	void	AddEventToHoldSymbols(const InputEvent &event);

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
	TInputDevices						Devices_;

	// CVars
	XInputCVars*						pCVars_;

	bool								enableEventPosting_;
	bool								retriggering_;

	bool								hasFocus_;

	ModifierFlags						modifiers_;	// caps ALT, SHIFT etc.

	InputSymbol InputSymbols_[input::KeyId::MOUSE_LAST];
};


X_NAMESPACE_END



#endif // !_X_INPUT_BASE_H_
