#include "stdafx.h"
#include "NullInput.h"

X_NAMESPACE_BEGIN(input)



void XNullInput::AddEventListener(IInputEventListner *pListener)
{
	X_UNUSED(pListener);
}

void XNullInput::RemoveEventListener(IInputEventListner *pListener)
{
	X_UNUSED(pListener);
}

void XNullInput::AddConsoleEventListener(IInputEventListner *pListener)
{
	X_UNUSED(pListener);
}

void XNullInput::RemoveConsoleEventListener(IInputEventListner *pListener)
{
	X_UNUSED(pListener);
}


bool XNullInput::AddInputDevice(IInputDevice* pDevice)
{ 
	X_UNUSED(pDevice);
	return false;
}

void XNullInput::EnableEventPosting(bool bEnable)
{
	X_UNUSED(bEnable);
}

bool XNullInput::IsEventPostingEnabled(void) const
{ 
	return false;
}

bool XNullInput::PostInputEvent(const InputEvent &event, bool bForce)
{
	X_UNUSED(event);
	X_UNUSED(bForce);
	return false; 
}


bool XNullInput::Init(void)
{
	return true; 
}

void XNullInput::PostInit(void)
{

}

void XNullInput::Update(bool bFocus)
{
	X_UNUSED(bFocus);
}

void XNullInput::ShutDown(void)
{

}

void XNullInput::release(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);
	X_DELETE(this, gEnv->pArena);
}


void XNullInput::ClearKeyState(void)
{

}

void XNullInput::RetriggerKeyState(void)
{

}

bool XNullInput::Retriggering(void)
{ 
	return false;
}


bool XNullInput::HasInputDeviceOfType(InputDeviceType::Enum type) const
{ 
	X_UNUSED(type);
	return false; 
}

void XNullInput::EnableDevice(InputDevice::Enum deviceId, bool enable)
{
	X_UNUSED(deviceId);
	X_UNUSED(enable);
}

XNullInput::ModifierFlags XNullInput::GetModifiers(void)
{ 
	return ModifierFlags(); 
}

void XNullInput::SetModifiers(ModifierFlags flags)
{
	X_UNUSED(flags);
}

InputSymbol* DefineSymbol(InputDevice::Enum device, KeyId::Enum id_, const KeyName& name_,
InputSymbol::Type type_, ModifiersMasks::Enum mod_mask)
{
	X_UNUSED(device);
	X_UNUSED(id_);
	X_UNUSED(name_);
	X_UNUSED(type_);
	X_UNUSED(mod_mask);
	return nullptr;
}


X_NAMESPACE_END