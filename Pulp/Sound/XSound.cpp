#include "stdafx.h"
#include "XSound.h"

X_DISABLE_WARNING(4505)

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

X_ENABLE_WARNING(4505)


// link all plugins
#define PLUGIN_All 0

#if PLUGIN_All == 0
#define PLUGIN_Codec 1
#define PLUGIN_Effect 1
#define PLUGIN_Rumble 0
#define PLUGIN_Source 1
#define PLUGIN_Auro 0
#else
#define PLUGIN_Codec 1
#define PLUGIN_Effect 1
#define PLUGIN_Rumble 1
#define PLUGIN_Source 1
#define PLUGIN_Auro 1
#endif // !PLUGIN_All

// link the libs..

// engine
X_LINK_LIB("AkMusicEngine");
X_LINK_LIB("AkSoundEngine");
X_LINK_LIB("AkMusicEngine");

// managers
X_LINK_LIB("AkMemoryMgr");
X_LINK_LIB("AkStreamMgr");


// decoeer / source?
#if PLUGIN_Codec
X_LINK_LIB("AkVorbisDecoder");
#endif // !PLUGIN_Codec

// source
X_LINK_LIB("AkSilenceSource");
X_LINK_LIB("AkMP3Source");
X_LINK_LIB("AkToneSource");
X_LINK_LIB("AkAudioInputSource");
X_LINK_LIB("AkSineSource");

// fx
X_LINK_LIB("AkConvolutionReverbFX");
X_LINK_LIB("AkFlangerFX");
X_LINK_LIB("AkTremoloFX");
#if PLUGIN_Auro
X_LINK_LIB("AuroHeadphoneFX");
#endif // !PLUGIN_Auro
X_LINK_LIB("AkStereoDelayFX");
X_LINK_LIB("AkGuitarDistortionFX");
X_LINK_LIB("AkSoundSeedImpactFX");
X_LINK_LIB("AkRoomVerbFX");
X_LINK_LIB("AkParametricEQFX");
X_LINK_LIB("AkCompressorFX");
X_LINK_LIB("AkPitchShifterFX");
X_LINK_LIB("AkPeakLimiterFX");
X_LINK_LIB("AkDelayFX");
X_LINK_LIB("AkGainFX");
X_LINK_LIB("AkMeterFX");
X_LINK_LIB("AkMatrixReverbFX");
X_LINK_LIB("AkHarmonizerFX");
X_LINK_LIB("AkTimeStretchFX");
X_LINK_LIB("AkExpanderFX");
X_LINK_LIB("McDSPFutzBoxFX");
X_LINK_LIB("McDSPLimiterFX");
X_LINK_LIB("CrankcaseAudioREVModelPlayerFX");

// fx2
#if 0 
X_LINK_LIB("iZTrashBoxModelerFX");
X_LINK_LIB("iZTrashDelayFX");
X_LINK_LIB("iZTrashMultibandDistortionFX");
X_LINK_LIB("iZHybridReverbFX");
X_LINK_LIB("iZTrashDynamicsFX");
X_LINK_LIB("iZTrashDistortionFX");
X_LINK_LIB("iZTrashFiltersFX");
#endif

// misc
#if PLUGIN_Auro
X_LINK_LIB("AuroPannerMixer");
#endif // !PLUGIN_Auro
X_LINK_LIB("AkSoundSeedWoosh");
X_LINK_LIB("AkSynthOne");
X_LINK_LIB("AkMotionGenerator");
X_LINK_LIB("AkSoundSeedWind");
X_LINK_LIB("IOSONOProximityMixer");

// rumble
#if PLUGIN_Rumble == 0
X_LINK_LIB("AkRumble");
#endif // !PLUGIN_Rumble


// devices
X_LINK_LIB("msacm32");
X_LINK_LIB("dxguid");


#if X_SUPER == 0
X_LINK_LIB("ws2_32"); // used for winsock
X_LINK_LIB("CommunicationCentral");
#endif // !X_SUPER

namespace AK
{
	core::MallocFreeAllocator akAlloca;

	void* AllocHook(size_t in_size)
	{
		return akAlloca.allocate(in_size, 0, 0);
	}
	void FreeHook(void * in_ptr)
	{
		akAlloca.free(in_ptr);
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

	void akAssertHook(const char* pszExpression, const char* pszFileName, int lineNumber)
	{
#if X_ENABLE_ASSERTIONS
		core::SourceInfo sourceInfo("SoundSys", pszFileName, lineNumber, "", "");
		core::Assert(sourceInfo, "Assertion \"%s\" failed.", pszExpression)
			.Variable("FileName", pszFileName)
			.Variable("LineNumber", lineNumber);

#else
		X_ERROR("SoundSys", "Sound system threw a assert: Exp: \"%s\" file: \"%s\" line: \"%s\"", 
			pszExpression, pszFileName, lineNumber);
#endif

		X_BREAKPOINT;
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

	AkDeviceSettings deviceSettings;
	StreamMgr::GetDefaultDeviceSettings(deviceSettings);

	AkInitSettings l_InitSettings;
	AkPlatformInitSettings l_platInitSetings;
	SoundEngine::GetDefaultInitSettings(l_InitSettings);
	SoundEngine::GetDefaultPlatformInitSettings(l_platInitSetings);

	AkMusicSettings musicInit;
	MusicEngine::GetDefaultInitSettings(musicInit);

	// Create and initialise an instance of our memory manager.
	if (MemoryMgr::Init(&memSettings) != AK_Success)
	{
		X_ERROR("SoundSys", "Could not create the memory manager.");
		return false;
	}

	// Create and initialise an instance of the default stream manager.
	if (!StreamMgr::Create(stmSettings))
	{
		X_ERROR("SoundSys", "Could not create the Stream Manager");
		return false;
	}

	// Create an IO device.
	deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
	if (ioHook_.Init(deviceSettings) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot create streaming I/O device");
		return false;
	}

	// Initialize sound engine.
	l_InitSettings.pfnAssertHook = akAssertHook;
	l_InitSettings.eMainOutputType = AkAudioAPI::AkAPI_Default;
	{
		const wchar_t* pOutputDevice = gEnv->pCore->GetCommandLineArgForVarW(L"snd_output_device");
		if (pOutputDevice)
		{
			if (core::strUtil::IsEqualCaseInsen(pOutputDevice, L"xaudio2")) {
				l_InitSettings.eMainOutputType = AkAudioAPI::AkAPI_XAudio2;
				X_LOG1("SoundSys", "using output device: XAudio2");
			}
			else if (core::strUtil::IsEqualCaseInsen(pOutputDevice, L"directsound")) {
				l_InitSettings.eMainOutputType = AkAudioAPI::AkAPI_DirectSound;
				X_LOG1("SoundSys", "using output device: directsound (DirectX Sound)");
			}
			else if (core::strUtil::IsEqualCaseInsen(pOutputDevice, L"wasapi")) {
				l_InitSettings.eMainOutputType = AkAudioAPI::AkAPI_Wasapi;
				X_LOG1("SoundSys", "using output device: Wasapi (Windows Audio Session)");
			}
			else if (core::strUtil::IsEqualCaseInsen(pOutputDevice, L"none")) {
				l_InitSettings.eMainOutputType = AkAudioAPI::AkAPI_Dummy;
				X_LOG1("SoundSys", "using output device: none (no sound)");
			}
			else {
				X_ERROR("SoundSys", "Unknown output device \"%ls\" using default instead. valid options: "
					"xaudio2, directsound, wasapi, none",
					pOutputDevice);
			}
		}
	}

	if (SoundEngine::Init(&l_InitSettings, &l_platInitSetings) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot initialize sound engine");
		return false;
	}

	// Initialize music engine.
	if (MusicEngine::Init(&musicInit) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot initialize music engine");
		return false;
	}

#if X_SUPER == 0

	// Initialize communication.
	AkCommSettings settingsComm;
	AK::Comm::GetDefaultInitSettings(settingsComm);
	AKPLATFORM::SafeStrCpy(settingsComm.szAppNetworkName, X_ENGINE_NAME, AK_COMM_SETTINGS_MAX_STRING_SIZE);
	if (AK::Comm::Init(settingsComm) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot initialize music communication");
		return false;
	}
#endif // !X_SUPER

	// Register plugins
#if !PLUGIN_All

	// what ones do i even want o.o

#if PLUGIN_Codec
	if (AK::SoundEngine::RegisterAllCodecPlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering codec plug-ins");
		return false;
	}
#endif // !PLUGIN_Codec

#if PLUGIN_Effect
	if (AK::SoundEngine::RegisterAllEffectPlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering effect plug-ins");
		return false;
	}
#endif // !PLUGIN_Effect

#if PLUGIN_Rumble
	if (AK::SoundEngine::RegisterAllRumblePlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering rumble plug-ins");
		return false;
	}
#endif // !PLUGIN_Rumble

#if PLUGIN_Source
	if (AK::SoundEngine::RegisterAllSourcePlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering souce plug-ins");
		return false;
	}
#endif // !PLUGIN_Source

#if PLUGIN_Auro
	if (AK::SoundEngine::RegisterAuroPlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering auro plug-ins");
		return false;
	}
#endif // !PLUGIN_Auro

#else
	/// Note: This a convenience method for rapid prototyping. 
	/// To reduce executable code size register/link only the plug-ins required by your game 
	if (AK::SoundEngine::RegisterAllPlugins() != AK_Success)
	{
		X_ERROR("SoundSys", "Error while registering plug-ins");
		return false;
	}
#endif

	return true;
}

void XSound::ShutDown(void)
{
	X_LOG0("SoundSys", "Shutting Down");
#if X_SUPER == 0
	Comm::Term();
#endif // !X_SUPER

	MusicEngine::Term();

	SoundEngine::Term();

	ioHook_.Term();

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


void XSound::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(lparam);

	if (event == CoreEvent::CHANGE_FOCUS)
	{
		if (wparam == 1) // activated
		{
			X_LOG2("SoundSys", "Suspending sound system");
			AK::SoundEngine::Suspend(false);
		}
		else
		{
			X_LOG2("SoundSys", "Waking sound system from syspend");

			AK::SoundEngine::WakeupFromSuspend();
			// might need to be called here not sure.
			// SoundEngine::RenderAudio();
		}
	}
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