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

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;

	void Update(void) X_FINAL;
	void StopAll(void) X_FINAL;

	// Shut up!
	void Mute(bool mute) X_FINAL;

	// Volume
	void SetMasterVolume(float v) X_FINAL;
	void SetMusicVolume(float vol) X_FINAL;
	void SetVoiceVolume(float vol) X_FINAL;
	void SetSFXVolume(float vol) X_FINAL;

	uint32_t GetIDFromStr(const char* pStr) X_FINAL;
	uint32_t GetIDFromStr(const wchar_t* pStr) X_FINAL;

	bool RegisterObject(GameObjectID object, const char* pNick) X_FINAL;
	bool UnRegisterObject(GameObjectID object) X_FINAL;
	
	void SetPosition(GameObjectID object, const Transformf& trans) X_FINAL;
	void SetPosition(GameObjectID* pObjects, const Transformf* pTrans, size_t num) X_FINAL;
	
	void PostEvent(EventID event, GameObjectID object) X_FINAL;

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
