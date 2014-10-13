#pragma once

#ifndef _X_NULL_INPUT_H_
#define _X_NULL_INPUT_H_

#include <IInput.h>

X_NAMESPACE_BEGIN(input)

class XNullInput : public IInput
{
public:
	virtual void AddEventListener(IInputEventListner *pListener) X_OVERRIDE{}
	virtual void RemoveEventListener(IInputEventListner *pListener) X_OVERRIDE{}

	virtual void AddConsoleEventListener(IInputEventListner *pListener) X_OVERRIDE{}
	virtual void RemoveConsoleEventListener(IInputEventListner *pListener) X_OVERRIDE{}

	virtual bool AddInputDevice(IInputDevice* pDevice) X_OVERRIDE{ return false; }

	virtual void EnableEventPosting(bool bEnable) X_OVERRIDE{}
	virtual bool IsEventPostingEnabled() const X_OVERRIDE{ return false; }
	virtual bool PostInputEvent(const InputEvent &event, bool bForce = false) X_OVERRIDE{ return false; }


	virtual bool Init() X_OVERRIDE{ return true; }
	virtual void PostInit() X_OVERRIDE{}
	virtual void Update(bool bFocus) X_OVERRIDE{}
	virtual void ShutDown() X_OVERRIDE{}
	virtual void release(void) X_OVERRIDE{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_DELETE(this, gEnv->pArena);
	}

	
	virtual void ClearKeyState() X_OVERRIDE{}

	virtual void RetriggerKeyState() X_OVERRIDE{}

	virtual bool Retriggering() X_OVERRIDE{ return false; }

	virtual bool HasInputDeviceOfType(InputDeviceType::Enum type) const X_OVERRIDE{ return false; }

	virtual void EnableDevice(InputDevice::Enum deviceId, bool enable) X_OVERRIDE{}

	virtual ModifierFlags GetModifiers() X_OVERRIDE{ return ModifierFlags(); }
	virtual void SetModifiers(ModifierFlags flags) X_OVERRIDE{}

	virtual InputSymbol* DefineSymbol(InputDevice::Enum device,
		KeyId::Enum id_, const KeyName& name_,
		InputSymbol::Type type_ = InputSymbol::Type::Button, 
		ModifiersMasks::Enum  mod_mask = ModifiersMasks::NONE) X_OVERRIDE{
		return nullptr; 
	}

};

X_NAMESPACE_END

#endif // ! _X_NULL_INPUT_H_
