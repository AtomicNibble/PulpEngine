#include "stdafx.h"
#include "SoundVars.h"

#include <IConsole.h>

#include "IDs\Wwise_IDs.h"

// Sound Engine
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

X_NAMESPACE_BEGIN(sound)



namespace
{

	void Var_MasterVolChanged(core::ICVar* pVar)
	{
		float vol = pVar->GetFloat();
		vol *= 255.f;

		AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::MASTERVOLUME, vol);
	}

	void Var_MusicVolChanged(core::ICVar* pVar)
	{
		float vol = pVar->GetFloat();
		vol *= 255.f;

		AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::MUSICVOLUME, vol);
	}

	void Var_SFXVolChanged(core::ICVar* pVar)
	{
		float vol = pVar->GetFloat();
		vol *= 255.f;

		AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::SFXVOLUME, vol);
	}

	void Var_VoiceVolChanged(core::ICVar* pVar)
	{
		float vol = pVar->GetFloat();
		vol *= 255.f;

		AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::VOICEVOLUME, vol);
	}


} // namespace

SoundVars::SoundVars()
{
	var_vol_master_ = nullptr;
	var_vol_music_ = nullptr;
	var_vol_sfx_ = nullptr;
	var_vol_voice_ = nullptr;
}

SoundVars::~SoundVars()
{
}


void SoundVars::RegisterVars(void)
{
	var_vol_master_ = ADD_CVAR_FLOAT("snd_vol_master", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Master volume");
	var_vol_music_ = ADD_CVAR_FLOAT("snd_vol_music", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Music volume");
	var_vol_sfx_ = ADD_CVAR_FLOAT("snd_vol_sfx", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "SFX volume");
	var_vol_voice_ = ADD_CVAR_FLOAT("snd_vol_voice", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Voice volume");


	var_vol_master_->SetOnChangeCallback(Var_MasterVolChanged);
	var_vol_music_->SetOnChangeCallback(Var_MusicVolChanged);
	var_vol_sfx_->SetOnChangeCallback(Var_SFXVolChanged);
	var_vol_voice_->SetOnChangeCallback(Var_VoiceVolChanged);


	ADD_CVAR_REF("snd_enable_coms", enableCommSys_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"initialize Wwise Comm system on startup");
	ADD_CVAR_REF("snd_enable_capture", enableOutputCapture_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Allows for capturing the output audio to a wav file.");
}



X_NAMESPACE_END
