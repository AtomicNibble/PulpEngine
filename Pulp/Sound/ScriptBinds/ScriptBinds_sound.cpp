#include "stdafx.h"
#include "ScriptBinds_sound.h"

#include "XSound.h"

X_NAMESPACE_BEGIN(sound)

ScriptBinds_Sound::ScriptBinds_Sound(script::IScriptSys* pSS, XSound* pSound) :
	IScriptBindsBase(pSS),
	pSound_(X_ASSERT_NOT_NULL(pSound))
{
	
}

ScriptBinds_Sound::~ScriptBinds_Sound()
{

}

void ScriptBinds_Sound::bind(void)
{
	createBindTable();
	setGlobalName("Sound");

	X_SCRIPT_BIND(ScriptBinds_Sound, PostEvent);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetSwitch);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetStages);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetParam);

	X_SCRIPT_BIND(ScriptBinds_Sound, SetMasterVol);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetMusicVol);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetVoiceVol);
	X_SCRIPT_BIND(ScriptBinds_Sound, SetSFXVol);
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


