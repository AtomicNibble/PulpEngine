#include "stdafx.h"
#include "SoundVars.h"

#include <IConsole.h>

#include "IDs\Wwise_IDs.h"

// Sound Engine
#include <AK/SoundEngine/Common/AkSoundEngine.h>

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
	enableCommSys_ = 0;
	enableOutputCapture_ = 0;
	enableCulling_ = 0;
	enableDebugRender_ = 0;

	debugObjectScale_ = 1.f;
	debugTextSize_ = 4.f;

	registeredCullDistance_ = 1024.f * 2.f; 
	occlusionRefreshRate_ = 0.2f;

	pVarVolMaster_ = nullptr;
	pVarVolMusic_ = nullptr;
	pVarVolSfx_ = nullptr;
	pVarVolVoice_ = nullptr;

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
	pVarVolMaster_ = ADD_CVAR_FLOAT("snd_vol_master", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Master volume");
	pVarVolMusic_ = ADD_CVAR_FLOAT("snd_vol_music", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Music volume");
	pVarVolSfx_ = ADD_CVAR_FLOAT("snd_vol_sfx", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "SFX volume");
	pVarVolVoice_ = ADD_CVAR_FLOAT("snd_vol_voice", 1.f, 0.f, 1.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Voice volume");

	core::ConsoleVarFunc del;
	del.Bind<Var_MasterVolChanged>();
	pVarVolMaster_->SetOnChangeCallback(del);

	del.Bind<Var_MusicVolChanged>();
	pVarVolMusic_->SetOnChangeCallback(del);

	del.Bind<Var_SFXVolChanged>();
	pVarVolSfx_->SetOnChangeCallback(del);

	del.Bind<Var_VoiceVolChanged>();
	pVarVolVoice_->SetOnChangeCallback(del);

	const int32_t maxVal = std::numeric_limits<int32_t>::max();

	ADD_CVAR_REF("snd_enable_coms", enableCommSys_, enableCommSys_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"initialize Wwise Comm system on startup");
	ADD_CVAR_REF("snd_enable_capture", enableOutputCapture_, enableOutputCapture_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Allows for capturing the output audio to a wav file.");
	ADD_CVAR_REF("snd_enable_cull", enableCulling_, enableCulling_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable inactive sound object culling");

	ADD_CVAR_REF("snd_debug_draw", enableDebugRender_, enableDebugRender_, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enables sound debug rendering. 1=pos/occ 2=info text");

	ADD_CVAR_REF("snd_debug_obj_scale", debugObjectScale_, debugObjectScale_, 1.f, 32.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Debug draw scale for objects");
	ADD_CVAR_REF("snd_debug_text_size", debugTextSize_, debugTextSize_, 0.001f, 128.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Debug draw text size");


	ADD_CVAR_REF("snd_active_cull_distance", registeredCullDistance_, registeredCullDistance_, 0.f, 1024.f * 16.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Distance active sound objects are culled. Inactive sound objects exceeding this distance will be put to sleep");
	ADD_CVAR_REF("snd_occlusion_refresh_rate", occlusionRefreshRate_, occlusionRefreshRate_, 0.f, 60.f,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The rate in seconds occlusion values are refreshed.");


	// LEEERORRRY Jenkins!!
	ADD_CVAR_REF("snd_mem_engine_default_pool", soundEngineDefaultMemoryPoolSize_, soundEngineDefaultMemoryPoolSize_, 0, maxVal, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine default memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_lower_default_pool", soundEngineLowerDefaultPoolSize_, soundEngineLowerDefaultPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine lower default memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_stream_pool", streamManagerMemoryPoolSize_, streamManagerMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine stream memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_stream_device_pool", streamDeviceMemoryPoolSize_, streamDeviceMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine stream device memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_command_queue_pool", commandQueueMemoryPoolSize_, commandQueueMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine command queue memory pool size (KiB).");

	ADD_CVAR_REF("snd_mem_engine_monitor_pool", monitorMemoryPoolSize_, monitorMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine monitor memory pool size (KiB).");
	ADD_CVAR_REF("snd_mem_engine_monitor_queue_pool", monitorQueueMemoryPoolSize_, monitorQueueMemoryPoolSize_, 0, maxVal,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
		"Sound engine monitor queue memory pool size (KiB).");
}


void SoundVars::setMasterVolume(float vol)
{
	pVarVolMaster_->Set(vol);
}

void SoundVars::setMusicVolume(float vol)
{
	pVarVolMusic_->Set(vol);
}

void SoundVars::setVoiceVolume(float vol)
{
	pVarVolSfx_->Set(vol);
}

void SoundVars::setSFXVolume(float vol)
{
	pVarVolVoice_->Set(vol);
}


X_NAMESPACE_END
