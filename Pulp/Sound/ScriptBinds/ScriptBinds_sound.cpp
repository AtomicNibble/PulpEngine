#include "stdafx.h"
#include "ScriptBinds_sound.h"

#include "XSound.h"

X_NAMESPACE_BEGIN(sound)

#define X_SOUND_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<ScriptBinds_Sound, &ScriptBinds_Sound::func>(this); \
	registerFunction(#func, Delegate); \
}


ScriptBinds_Sound::ScriptBinds_Sound(script::IScriptSys* pSS, XSound* pSound) :
	pSound_(X_ASSERT_NOT_NULL(pSound))
{
	init(pSS);
}

ScriptBinds_Sound::~ScriptBinds_Sound()
{

}

void ScriptBinds_Sound::init(script::IScriptSys* pSS)
{
	XScriptableBase::init(pSS);

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


int32_t ScriptBinds_Sound::PostEvent(script::IFunctionHandler* pH)
{

	

	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetSwitch(script::IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetStages(script::IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetParam(script::IFunctionHandler* pH)
{
	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetMasterVol(script::IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSound_->setMasterVolume(vol);
	}

	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetMusicVol(script::IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSound_->setMusicVolume(vol);
	}

	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetVoiceVol(script::IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSound_->setVoiceVolume(vol);
	}

	return pH->endFunction();
}

int32_t ScriptBinds_Sound::SetSFXVol(script::IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	float vol;

	if (pH->getParam(0, vol)) {
		pSound_->setSFXVolume(vol);
	}

	return pH->endFunction();
}



X_NAMESPACE_END


