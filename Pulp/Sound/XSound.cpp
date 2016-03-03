#include "stdafx.h"
#include "XSound.h"

// Sound Engine
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

// Music engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>

// Tools common
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAssert.h>

// Comms
#include <AK/Comm/AkCommunication.h>

// Plugin
#include <AK/Plugin/AllPluginsRegistrationHelpers.h>

// link the libs..
X_LINK_LIB("AkSoundEngine");
X_LINK_LIB("AkMemoryMgr");
X_LINK_LIB("AkMusicEngine");
X_LINK_LIB("AkStreamMgr");
X_LINK_LIB("AkConvolutionReverbFX");
X_LINK_LIB("AkFlangerFX");
X_LINK_LIB("AkTremoloFX");
X_LINK_LIB("AuroHeadphoneFX");
X_LINK_LIB("AkMotionGenerator");
X_LINK_LIB("AkSineSource");
X_LINK_LIB("AkSoundSeedWind");
X_LINK_LIB("AkStereoDelayFX");
X_LINK_LIB("AkGuitarDistortionFX");
X_LINK_LIB("AkRumble");
X_LINK_LIB("AkSilenceSource");
X_LINK_LIB("AkSoundSeedImpactFX");
X_LINK_LIB("AkRoomVerbFX");
X_LINK_LIB("McDSPFutzBoxFX");
X_LINK_LIB("AkParametricEQFX");
X_LINK_LIB("AuroPannerMixer");
X_LINK_LIB("AkToneSource");
X_LINK_LIB("McDSPLimiterFX");
X_LINK_LIB("AkCompressorFX");
X_LINK_LIB("AkAudioInputSource");
X_LINK_LIB("AkMusicEngine");
X_LINK_LIB("AkSoundSeedWoosh");
X_LINK_LIB("AkPitchShifterFX");
X_LINK_LIB("AkPeakLimiterFX");
X_LINK_LIB("AkDelayFX");
X_LINK_LIB("AkGainFX");
X_LINK_LIB("AkVorbisDecoder");
X_LINK_LIB("AkMeterFX");
X_LINK_LIB("AkMatrixReverbFX");
X_LINK_LIB("AkSynthOne");
X_LINK_LIB("AkMP3Source");
X_LINK_LIB("AkHarmonizerFX");
X_LINK_LIB("AkTimeStretchFX");
X_LINK_LIB("AkExpanderFX");

X_LINK_LIB("IOSONOProximityMixer");
X_LINK_LIB("CrankcaseAudioREVModelPlayerFX");
X_LINK_LIB("CommunicationCentral");

X_LINK_LIB("iZTrashBoxModelerFX");
X_LINK_LIB("iZTrashDelayFX");
X_LINK_LIB("iZTrashMultibandDistortionFX");
X_LINK_LIB("iZHybridReverbFX");
X_LINK_LIB("iZTrashDynamicsFX");
X_LINK_LIB("iZTrashDistortionFX");
X_LINK_LIB("iZTrashFiltersFX");

X_LINK_LIB("ws2_32");
X_LINK_LIB("msacm32");
X_LINK_LIB("dxguid");


namespace AK
{
	void* AllocHook(size_t in_size)
	{
		return malloc(in_size);
	}
	void FreeHook(void * in_ptr)
	{
		free(in_ptr);
	}

	void* VirtualAllocHook(
		void* in_pMemAddress,
		size_t in_size,
		DWORD in_dwAllocationType,
		DWORD in_dwProtect
		)
	{
		return ::VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
	}
	void VirtualFreeHook(
		void* in_pMemAddress,
		size_t in_size,
		DWORD in_dwFreeType
		)
	{
		::VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
	}


}

using namespace AK;

X_NAMESPACE_BEGIN(sound)


XSound::XSound()
{

}

XSound::~XSound()
{
}


bool XSound::Init(void)
{
	X_LOG0("SoundSys", "Starting");

	AkMemSettings memSettings;

	memSettings.uMaxNumPools = 20;

	// Streaming.
	AkStreamMgrSettings stmSettings;
	StreamMgr::GetDefaultSettings(stmSettings);

//	AkDeviceSettings deviceSettings;
//	deviceSettings.
//	StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	AkInitSettings l_InitSettings;
	AkPlatformInitSettings l_platInitSetings;
	SoundEngine::GetDefaultInitSettings(l_InitSettings);
	SoundEngine::GetDefaultPlatformInitSettings(l_platInitSetings);

	AkMusicSettings musicInit;
	MusicEngine::GetDefaultInitSettings(musicInit);

	// Create and initialise an instance of our memory manager.
	if (MemoryMgr::Init(&memSettings) != AK_Success)
	{
		AKASSERT(!"Could not create the memory manager.");
		return false;
	}

	// Create and initialise an instance of the default stream manager.
	if (!StreamMgr::Create(stmSettings))
	{
		AKASSERT(!"Could not create the Stream Manager");
		return false;
	}

#if 0
	// Create an IO device.
	if (g_lowLevelIO.Init(deviceSettings) != AK_Success)
	{
		AKASSERT(!"Cannot create streaming I/O device");
		return false;
	}
#endif

	// Initialize sound engine.
	if (SoundEngine::Init(&l_InitSettings, &l_platInitSetings) != AK_Success)
	{
		AKASSERT(!"Cannot initialize sound engine");
		return false;
	}

	// Initialize music engine.
	if (MusicEngine::Init(&musicInit) != AK_Success)
	{
		AKASSERT(!"Cannot initialize music engine");
		return false;
	}

	// Initialize communication.
	AkCommSettings settingsComm;
	AK::Comm::GetDefaultInitSettings(settingsComm);
	AKPLATFORM::SafeStrCpy(settingsComm.szAppNetworkName, X_ENGINE_NAME, AK_COMM_SETTINGS_MAX_STRING_SIZE);
	if (AK::Comm::Init(settingsComm) != AK_Success)
	{
		AKASSERT(!"Cannot initialize music communication");
		return false;
	}

	// Register plugins
	/// Note: This a convenience method for rapid prototyping. 
	/// To reduce executable code size register/link only the plug-ins required by your game 
	if (AK::SoundEngine::RegisterAllPlugins() != AK_Success)
	{
		AKASSERT(!"Error while registering plug-ins");
		return false;
	}

	return true;
}

void XSound::ShutDown(void)
{
	X_LOG0("SoundSys", "Shutting Down");

	Comm::Term();

	MusicEngine::Term();

	SoundEngine::Term();

//	g_lowLevelIO.Term();

	if (IAkStreamMgr::Get()) {
		IAkStreamMgr::Get()->Destroy();
	}

	MemoryMgr::Term();
}

void XSound::release(void)
{
	X_DELETE(this,g_SoundArena);
}

void XSound::Update(void)
{
	X_PROFILE_BEGIN("SoundUpdate", core::ProfileSubSys::SOUND);

	SoundEngine::RenderAudio();
}


// Shut up!
void XSound::Mute(bool mute)
{
	X_UNUSED(mute);
}

// Volume
void XSound::SetMasterVolume(float v)
{
	// nope
	X_UNUSED(v);
}

float XSound::GetMasterVolume(void) const
{
	return 1.f; // MAX VOLUME ALL THE TIME!!!
}


X_NAMESPACE_END