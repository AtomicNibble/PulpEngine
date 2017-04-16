#pragma once


#ifndef _X_SOUNG_I_H_
#define _X_SOUNG_I_H_

#include "IO\AkIoHook.h"
#include "Vars\SoundVars.h"

#include <ICore.h>

X_NAMESPACE_DECLARE(core,
struct ICVar;
struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(sound)

class XSound : public ISound, public ICoreEventListener
{
public:
	XSound();
	virtual ~XSound() X_OVERRIDE;

	virtual void registerVars(void) X_FINAL;
	virtual void registerCmds(void) X_FINAL;

	virtual bool init(void) X_FINAL;
	virtual void shutDown(void) X_FINAL;
	virtual void release(void) X_FINAL;

	virtual void Update(void) X_FINAL;
	virtual void StopAll(void) X_FINAL;

	// Shut up!
	virtual void Mute(bool mute) X_FINAL;

	// Volume
	virtual void SetMasterVolume(float v) X_FINAL;
	virtual void SetMusicVolume(float vol) X_FINAL;
	virtual void SetVoiceVolume(float vol) X_FINAL;
	virtual void SetSFXVolume(float vol) X_FINAL;

private:

private:
	void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;

private:
	void cmd_SetRtpc(core::IConsoleCmdArgs* pArgs);
	void cmd_SetSwitchState(core::IConsoleCmdArgs* pArgs);
	void cmd_PostEvent(core::IConsoleCmdArgs* pArgs);
	void cmd_StopEvent(core::IConsoleCmdArgs* pArgs);
	void cmd_StopAllEvent(core::IConsoleCmdArgs* pArgs);

private:
	IOhook ioHook_;

private:
	AkGameObjectID globalObjID_;
	AkBankID initBankID_;

	SoundVars vars_;

	bool comsSysInit_;
	bool outputCaptureEnabled_;

};

X_NAMESPACE_END

#endif // !_X_SOUNG_I_H_
