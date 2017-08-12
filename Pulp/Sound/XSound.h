#pragma once


#ifndef _X_SOUNG_I_H_
#define _X_SOUNG_I_H_

#include "IO\AkIoHook.h"
#include "Vars\SoundVars.h"
#include "Util\Allocators.h"

#include <Threading\Signal.h>

#include <ICore.h>

X_NAMESPACE_DECLARE(core,
struct ICVar;
struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(sound)

class XSound : public ISound, public ICoreEventListener
{
	struct Bank
	{
		X_DECLARE_ENUM(Status)(
			Loading,
			Loaded,
			Unloading,
			Error
		);

		Status::Enum status;
		AkBankID bankID;
		core::string name;
	};

	typedef core::Array<Bank> BanksArr;

public:
	XSound();
	virtual ~XSound() X_OVERRIDE;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(void) X_FINAL;
	bool asyncInitFinalize(void) X_FINAL;
	void shutDown(void) X_FINAL;
	void release(void) X_FINAL;

	void Update(void) X_FINAL;

	// Shut up!
	void mute(bool mute) X_FINAL;

	void setListenPos(const Transformf& trans) X_FINAL;

	// Volume
	void setMasterVolume(float v) X_FINAL;
	void setMusicVolume(float vol) X_FINAL;
	void setVoiceVolume(float vol) X_FINAL;
	void setSFXVolume(float vol) X_FINAL;

	uint32_t getIDFromStr(const char* pStr) const X_FINAL;
	uint32_t getIDFromStr(const wchar_t* pStr) const X_FINAL;

	bool registerObject(GameObjectID object, const char* pNick) X_FINAL;
	bool registerObject(GameObjectID object, const Transformf& trans, const char* pNick) X_FINAL;
	bool unRegisterObject(GameObjectID object) X_FINAL;
	void unRegisterAll(void) X_FINAL;

	void setPosition(GameObjectID object, const Transformf& trans) X_FINAL;
	void setPosition(GameObjectID* pObjects, const Transformf* pTrans, size_t num) X_FINAL;
	
	void stopAll(GameObjectID object) X_FINAL;

	void postEvent(EventID event, GameObjectID object) X_FINAL;
	void postEvent(const char* pEventStr, GameObjectID object) X_FINAL;


	void setMaterial(GameObjectID object, engine::MaterialSurType::Enum surfaceType) X_FINAL;
	void setSwitch(SwitchGroupID group, SwitchStateID state, GameObjectID object) X_FINAL;
	void setRTPCValue(RtpcID id, RtpcValue val, GameObjectID object = INVALID_OBJECT_ID,
		core::TimeVal changeDuration = core::TimeVal(0ll), 
		CurveInterpolation::Enum fadeCurve = CurveInterpolation::Linear) X_FINAL;

	AkBankID getBankId(const char* pName) const;
	Bank* getBankForID(AkBankID id);
	void loadBank(const char* pName) X_FINAL;
	void unLoadBank(const char* pName) X_FINAL;
	bool waitForBankLoad(Bank& bank);

	void listBanks(const char* pSearchString) const;

private:
	static void bankCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId, void* pCookie);
	void bankCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId);

	static void bankUnloadCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId, void* pCookie);
	void bankUnloadCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId);

private:
	void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;

private:
	void cmd_SetRtpc(core::IConsoleCmdArgs* pArgs);
	void cmd_SetSwitchState(core::IConsoleCmdArgs* pArgs);
	void cmd_PostEvent(core::IConsoleCmdArgs* pArgs);
	void cmd_StopEvent(core::IConsoleCmdArgs* pArgs);
	void cmd_StopAllEvent(core::IConsoleCmdArgs* pArgs);
	void cmd_ListBanks(core::IConsoleCmdArgs* pArgs);

private:
	AllocatorHooks allocators_;
	IOhook ioHook_;

private:
	SoundVars vars_;
	
	mutable core::CriticalSection cs_;
	BanksArr banks_;


	bool comsSysInit_;
	bool outputCaptureEnabled_;

	Transformf listenerTrans_;

	core::Signal bankSignal_;
};

X_NAMESPACE_END

#endif // !_X_SOUNG_I_H_
