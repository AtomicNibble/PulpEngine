#pragma once


#ifndef _X_SOUND_I_H_
#define _X_SOUND_I_H_

X_NAMESPACE_BEGIN(sound)

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


};

X_NAMESPACE_END

#endif // !_X_SOUND_I_H_
