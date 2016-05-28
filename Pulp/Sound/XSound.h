#pragma once


#ifndef _X_SOUNG_I_H_
#define _X_SOUNG_I_H_

#include "IO\AkIoHook.h"
#include "Vars\SoundVars.h"

#include <ICore.h>

X_NAMESPACE_DECLARE(core,
struct ICVar;
)

X_NAMESPACE_BEGIN(sound)

class XSound : public ISound, public ICoreEventListener
{
public:
	XSound();
	virtual ~XSound();

	virtual void RegisterVars(void) X_OVERRIDE;

	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual void release(void) X_OVERRIDE;

	virtual void Update(void) X_OVERRIDE;

	// Shut up!
	virtual void Mute(bool mute) X_OVERRIDE;

	// Volume
	virtual void SetMasterVolume(float v) X_OVERRIDE;
	virtual void SetMusicVolume(float vol) X_OVERRIDE;
	virtual void SetVoiceVolume(float vol) X_OVERRIDE;
	virtual void SetSFXVolume(float vol) X_OVERRIDE;

private:

private:
	void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;


private:
	IOhook ioHook_;

private:
	SoundVars vars_;

	bool comsSysInit_;
	bool outputCaptureEnabled_;

};

X_NAMESPACE_END

#endif // !_X_SOUNG_I_H_
