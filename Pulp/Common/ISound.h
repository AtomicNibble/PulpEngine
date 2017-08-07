#pragma once


#ifndef _X_SOUND_I_H_
#define _X_SOUND_I_H_

X_NAMESPACE_BEGIN(sound)


typedef uintptr_t GameObjectID;
typedef uint32_t EventID;

static const GameObjectID GLOBAL_OBJECT_ID = static_cast<GameObjectID>(-2);

struct ISound : public core::IEngineSysBase
{
	virtual ~ISound(){};

	// ting tong wong, sing me a song in a thong!
	virtual void Update(void) X_ABSTRACT;
	virtual void StopAll(void) X_ABSTRACT;

	// Shut up!
	virtual void Mute(bool mute) X_ABSTRACT;

	// Volume
	virtual void SetMasterVolume(float vol) X_ABSTRACT;
	virtual void SetMusicVolume(float vol) X_ABSTRACT;
	virtual void SetVoiceVolume(float vol) X_ABSTRACT;
	virtual void SetSFXVolume(float vol) X_ABSTRACT;

	// the id is passed in, so could just pass pointer value in then use that as id.
	virtual bool RegisterObject(GameObjectID object, const char* pNick = nullptr) X_ABSTRACT;
	virtual bool UnRegisterObject(GameObjectID object) X_ABSTRACT;

	virtual void SetPosition(GameObjectID object, const Transformf& trans) X_ABSTRACT;
	virtual void SetPosition(GameObjectID* pObjects, const Transformf* pTrans, size_t num) X_ABSTRACT;

	// events
	virtual void PostEvent(EventID event, GameObjectID object) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !_X_SOUND_I_H_
