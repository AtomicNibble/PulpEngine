#include "stdafx.h"
#include "ScriptBinds_sound.h"

#include <ISound.h>


X_NAMESPACE_BEGIN(script)

using namespace sound;

#define X_SOUND_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Sound, &XBinds_Sound::func>(this); \
	registerFunction(#func, Delegate); \
}


XBinds_Sound::XBinds_Sound()
{
}

XBinds_Sound::~XBinds_Sound()
{

}

void XBinds_Sound::init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	XScriptableBase::init(pSS, pCore, paramIdOffset);
	
	X_ASSERT_NOT_NULL(pCore->GetISound());

	pSoundSys_ = pCore->GetISound();

	setGlobalName("Sound");

	X_SOUND_REG_FUNC(PostEvent);
	X_SOUND_REG_FUNC(SetSwitch);
	X_SOUND_REG_FUNC(SetStages);
	X_SOUND_REG_FUNC(SetParam);

	X_SOUND_REG_FUNC(SetMasterVol);
	X_SOUND_REG_FUNC(SetMusicVol);
	X_SOUND_REG_FUNC(SetVoiceVol);
	X_SOUND_REG_FUNC(SetSFXVol);
}

int32_t XBinds_Sound::PostEvent(IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t XBinds_Sound::SetSwitch(IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t XBinds_Sound::SetStages(IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t XBinds_Sound::SetParam(IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t XBinds_Sound::SetMasterVol(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSoundSys_->setMasterVolume(vol);
	}

	return pH->endFunction();
}

int32_t XBinds_Sound::SetMusicVol(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSoundSys_->setMusicVolume(vol);
	}

	return pH->endFunction();
}

int32_t XBinds_Sound::SetVoiceVol(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSoundSys_->setVoiceVolume(vol);
	}

	return pH->endFunction();
}

int32_t XBinds_Sound::SetSFXVol(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSoundSys_->setSFXVolume(vol);
	}

	return pH->endFunction();
}


/*

int XBinds_Sound::Play(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_event>


	return pH->endFunction();
}

int XBinds_Sound::PlayEx(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_event>

	return pH->endFunction();
}
*/



X_NAMESPACE_END
