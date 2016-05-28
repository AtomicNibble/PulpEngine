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

	soundEngineDefaultMemoryPoolSize_ = 16 << 10;	// 16 MiB
	soundEngineLowerDefaultPoolSize_ = 16 << 10;	// 16 MiB

	streamManagerMemoryPoolSize_ = 64;				// 64 KiB
	streamDeviceMemoryPoolSize_ = 2 << 10;			// 2 MiB

	commandQueueMemoryPoolSize_ = 256;				// 256 KiB

	monitorMemoryPoolSize_ = 256;					// 256 KiB
	monitorQueueMemoryPoolSize_ = 64;				// 64 KiB
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

	const int32_t maxVal = std::numeric_limits<int32_t>::max();

	ADD_CVAR_REF("snd_enable_coms", enableCommSys_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"initialize Wwise Comm system on startup");
	ADD_CVAR_REF("snd_enable_capture", enableOutputCapture_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Allows for capturing the output audio to a wav file.");

	// LEEERORRRY Jenkins!!
	ADD_CVAR_REF("snd_mem_engine_default_pool", soundEngineDefaultMemoryPoolSize_, soundEngineDefaultMemoryPoolSize_, 0, maxVal, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine default memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_lower_default_pool", soundEngineLowerDefaultPoolSize_, soundEngineLowerDefaultPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine lower default memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_stream_pool", streamManagerMemoryPoolSize_, streamManagerMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine stream memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_stream_device_pool", streamDeviceMemoryPoolSize_, streamDeviceMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine stream device memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_command_queue_pool", commandQueueMemoryPoolSize_, commandQueueMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine command queue memory pool size (KiB).");

	ADD_CVAR_REF("snd_mem_engine_monitor_pool", monitorMemoryPoolSize_, monitorMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine monitor memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_monitor_queue_pool", monitorQueueMemoryPoolSize_, monitorQueueMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Sound engine monitor queue memory pool size (KiB).");
}



X_NAMESPACE_END
