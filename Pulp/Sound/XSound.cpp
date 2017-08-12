#include "stdafx.h"
#include "XSound.h"

#include <Memory\SimpleMemoryArena.h>

// id's
#include "IDs\Wwise_IDs.h"

#include <IConsole.h>

X_DISABLE_WARNING(4505)

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
#define PLUGIN_Source_mp3 0
#define PLUGIN_Auro 0
#else
#define PLUGIN_Codec 1
#define PLUGIN_Effect 1
#define PLUGIN_Rumble 1
#define PLUGIN_Source 1
#define PLUGIN_Source_mp3 1
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
#if PLUGIN_Source_mp3
X_LINK_LIB("AkMP3Source");
#endif // !PLUGIN_Source_mp3
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
#if PLUGIN_Source_mp3
X_LINK_LIB("msacm32");
#endif // !PLUGIN_Source_mp3
X_LINK_LIB("dxguid");


#if X_SUPER == 0
X_LINK_LIB("ws2_32"); // used for winsock
X_LINK_LIB("CommunicationCentral");
#endif // !X_SUPER

namespace AK
{
	core::MallocFreeAllocator akAlloca;
	core::SimpleMemoryArena<core::MallocFreeAllocator> akArena(&akAlloca, "AKArena");

	// these are unresolved symbols in ak, so they get use just be been defined.
	void* AllocHook(size_t in_size)
	{
		return akAlloca.allocate(in_size, 1, 0);
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

static_assert(GLOBAL_OBJECT_ID == static_cast<GameObjectID>(-2), "Should be negative 2 yo");
static_assert(INVALID_OBJECT_ID == AK_INVALID_GAME_OBJECT, "Invalid id incorrect");

XSound::XSound() :
	banks_(g_SoundArena),
	comsSysInit_(false),
	outputCaptureEnabled_(false),
	bankSignal_(true)
{
	// link to arena tree.
	g_SoundArena->addChildArena(&akArena);
}

XSound::~XSound()
{

}



void XSound::registerVars(void)
{
	vars_.RegisterVars();
}

void XSound::registerCmds(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);


	ADD_COMMAND_MEMBER("snd_set_rtpc", this, XSound, &XSound::cmd_SetRtpc, core::VarFlag::SYSTEM,
		"Set a audio RTPC value. <name> <state> <ObjectId>");
	ADD_COMMAND_MEMBER("snd_set_switchstate", this, XSound, &XSound::cmd_SetSwitchState, core::VarFlag::SYSTEM,
		"Set a audio Switch State. <name> <state> <ObjectId>");

	ADD_COMMAND_MEMBER("snd_post_event", this, XSound, &XSound::cmd_PostEvent, core::VarFlag::SYSTEM,
		"Post a audio event. <eventName> <ObjectId>");
	ADD_COMMAND_MEMBER("snd_stop_event", this, XSound, &XSound::cmd_StopEvent, core::VarFlag::SYSTEM,
		"Stop a audio event on a object. <eventName> <ObjectId>");

	ADD_COMMAND_MEMBER("snd_stop_all_event", this, XSound, &XSound::cmd_StopAllEvent, core::VarFlag::SYSTEM,
		"Stop all audio events for a object. <ObjectId>");

	ADD_COMMAND_MEMBER("listSndBanks", this, XSound, &XSound::cmd_ListBanks, core::VarFlag::SYSTEM,
		"List all the loaded sound banks");

}



bool XSound::init(void)
{
	X_LOG0("SoundSys", "Starting");
	X_PROFILE_NO_HISTORY_BEGIN("SoundInit", core::profiler::SubSys::SOUND);

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
	stmSettings.uMemorySize = vars_.StreamManagerMemoryPoolBytes();
	if (!StreamMgr::Create(stmSettings))
	{
		X_ERROR("SoundSys", "Could not create the Stream Manager");
		return false;
	}

	// Create an IO device.
	deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
	deviceSettings.uIOMemorySize = vars_.StreamDeviceMemoryPoolBytes();
	if (ioHook_.Init(deviceSettings, true) != AK_Success)
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


	l_InitSettings.uDefaultPoolSize = vars_.SoundEngineDefaultMemoryPoolBytes();
	l_InitSettings.uCommandQueueSize = vars_.CommandQueueMemoryPoolBytes();
	l_InitSettings.uMonitorPoolSize = vars_.MonitorMemoryPoolBytes();
	l_InitSettings.uMonitorQueuePoolSize = vars_.MonitorQueueMemoryPoolBytes();

	l_platInitSetings.uLEngineDefaultPoolSize = vars_.SoundEngineLowerDefaultMemoryPoolBytes();

	if (SoundEngine::Init(&l_InitSettings, &l_platInitSetings) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot initialize sound engine");
		return false;
	}

	// set the seed.
	{
		auto& sv = gEnv->seed;
		auto seed = sv[0] ^ sv[1] ^ sv[2] ^ sv[3];
		SoundEngine::SetRandomSeed(seed);
	}

	// Initialize music engine.
	if (MusicEngine::Init(&musicInit) != AK_Success)
	{
		X_ERROR("SoundSys", "Cannot initialize music engine");
		return false;
	}

#if X_SUPER == 0

	if (vars_.EnableComs())
	{
		// Initialize communication.
		AkCommSettings settingsComm;
		AK::Comm::GetDefaultInitSettings(settingsComm);
		AKPLATFORM::SafeStrCpy(settingsComm.szAppNetworkName, X_ENGINE_NAME, AK_COMM_SETTINGS_MAX_STRING_SIZE);
		if (AK::Comm::Init(settingsComm) != AK_Success)
		{
			X_ERROR("SoundSys", "Cannot initialize Wwise communication");
			return false;
		}

		comsSysInit_ = true;
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
	{
		if(AK::SoundEngine::RegisterPlugin(
			AkPluginTypeSource,
			AKCOMPANYID_AUDIOKINETIC,
			AKSOURCEID_SILENCE,
			CreateSilenceSource,
			CreateSilenceSourceParams) != AK_Success)
		{
			X_ERROR("SoundSys", "Error while registering silence souce plug-in");
			return false;
		}

		if(AK::SoundEngine::RegisterPlugin(
			AkPluginTypeSource,
			AKCOMPANYID_AUDIOKINETIC,
			AKSOURCEID_SINE,
			CreateSineSource,
			CreateSineSourceParams) != AK_Success)
		{
			X_ERROR("SoundSys", "Error while registering sine souce plug-in");
			return false;
		}

		if(AK::SoundEngine::RegisterPlugin(
			AkPluginTypeSource,
			AKCOMPANYID_AUDIOKINETIC,
			AKSOURCEID_TONE,
			CreateToneSource,
			CreateToneSourceParams) != AK_Success)
		{
			X_ERROR("SoundSys", "Error while registering tone souce plug-in");
			return false;
		}

#if defined( AK_WIN ) && !defined( AK_USE_METRO_API ) && PLUGIN_Source_mp3 
		if(AK::SoundEngine::RegisterPlugin(
			AkPluginTypeSource,
			AKCOMPANYID_AUDIOKINETIC,
			AKSOURCEID_MP3,
			CreateMP3Source,
			CreateMP3SourceParams) != AK_Success)
		{
			X_ERROR("SoundSys", "Error while registering mp3 souce plug-in");
			return false;
		}
#endif

		if(AK::SoundEngine::RegisterPlugin(
			AkPluginTypeSource,
			AKCOMPANYID_AUDIOKINETIC,
			AKSOURCEID_SYNTHONE,
			CreateSynthOne,
			CreateSynthOneParams) != AK_Success)
		{
			X_ERROR("SoundSys", "Error while registering synthone souce plug-in");
			return false;
		}
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


	AKRESULT res;

	// register a object for stuff with no position.
	res = AK::SoundEngine::RegisterGameObj(GLOBAL_OBJECT_ID, "GlobalObject");
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error creating global object. %s", AkResult::ToString(res, desc));
		return false;
	}

	// dispatch async loads for base banks.
	loadBank("Init.bnk");
	loadBank("Events.bnk");

	// temp
	loadBank("PlayerSounds.bnk");
	loadBank("Ambient.bnk");


	return true;
}

bool XSound::asyncInitFinalize(void)
{
	bool loaded = true;

	for (auto& bank : banks_)
	{
		loaded &= waitForBankLoad(bank);
	}

	return loaded;
}


AkBankID XSound::getBankId(const char* pName) const
{
	core::Path<char> name(pName);
	name.toLower();
	name.removeExtension();
	
	return GetIDFromStr(name.c_str());
}

XSound::Bank* XSound::getBankForID(AkBankID id)
{
	for (auto& bank : banks_)
	{
		if (bank.bankID == id) {
			return &bank;
		}
	}

	return nullptr;
}

void XSound::loadBank(const char* pName)
{
	core::CriticalSection::ScopedLock lock(cs_);

	auto bankID = getBankId(pName);

	Bank* pBank = getBankForID(bankID);
	if (pBank && pBank->status != Bank::Status::Error) {
		X_ERROR("SoundSys", "bank \"%s\" it's already loaded", pName);
		return;
	}
	else {
		pBank = &banks_.AddOne();
		pBank->name = pName;
		pBank->bankID = bankID;
	}

	pBank->status = Bank::Status::Loading;

	AKRESULT res = SoundEngine::LoadBank(pName, bankCallbackFunc_s, this, AK_DEFAULT_POOL_ID, pBank->bankID);
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Failed to load bank \"\" %s", pName, AkResult::ToString(res, desc));
	}

	X_ASSERT(pBank->bankID == bankID, "Bank id mismatch")(pBank->bankID, bankID);
}

void XSound::unLoadBank(const char* pName)
{
	core::CriticalSection::ScopedLock lock(cs_);

	auto bankID = getBankId(pName);
	Bank* pBank = getBankForID(bankID);
	if (!pBank) {
		X_ERROR("SoundSys", "Can't unload bank \"%s\" it's not loaded", pName);
		return;
	}

	if (pBank->status != Bank::Status::Loaded) {
		X_ERROR("SoundSys", "Can't unload bank \"%s\" it's not fully loaded", pName);
		return;
	}

	AKRESULT res = SoundEngine::UnloadBank(bankID, nullptr, bankUnloadCallbackFunc_s, this);
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Failed to un-load bank \"\" %s", pName, AkResult::ToString(res, desc));
	}
}

bool XSound::waitForBankLoad(Bank& bank)
{
	while (bank.status == Bank::Status::Loading) {
		bankSignal_.wait();
	}

	return bank.status == Bank::Status::Loaded;
}

void XSound::listBanks(const char* pSearchPatten) const
{
	core::CriticalSection::ScopedLock lock(cs_);

	core::Array<const Bank*> sorted_banks(g_SoundArena);

	for (const auto& bank : banks_)
	{
		if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, bank.name))
		{
			sorted_banks.push_back(&bank);
		}
	}

	std::sort(sorted_banks.begin(), sorted_banks.end(), [](const Bank* a, const Bank* b) {
			return a->name.compareInt(b->name) < 0;
		}
	);

	X_LOG0("SoundSys", "------------ ^8Banks(%" PRIuS ")^7 ---------------", sorted_banks.size());

	for (const auto* pBank : sorted_banks)
	{
		X_LOG0("SoundSys", "^2%-32s^7 ID:^2%-10i^7 Status:^2%s",
			pBank->name.c_str(), pBank->bankID, Bank::Status::ToString(pBank->status));
	}

	X_LOG0("SoundSys", "------------ ^8Banks End^7 --------------");
}

void XSound::bankCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
	AkMemPoolId	memPoolId, void* pCookie)
{
	reinterpret_cast<XSound*>(pCookie)->bankCallbackFunc(bankID, pInMemoryBankPtr, eLoadResult, memPoolId);
}


void XSound::bankCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
	AkMemPoolId	memPoolId)
{
	X_UNUSED(pInMemoryBankPtr, memPoolId);
	Bank* pBank = X_ASSERT_NOT_NULL(getBankForID(bankID));

	if (eLoadResult == AK_Success)
	{
		pBank->status = Bank::Status::Loaded;
		X_LOG0("SoundSys", "bank \"%s\" loaded", pBank->name.c_str());
	}
	else
	{
		pBank->status = Bank::Status::Error;

		AkResult::Description desc;
		X_LOG0("SoundSys", "bank \"%s\" failed to load. %s", pBank->name.c_str(), AkResult::ToString(eLoadResult, desc));
	}

	bankSignal_.raise();
}

void XSound::bankUnloadCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
	AkMemPoolId	memPoolId, void* pCookie)
{
	reinterpret_cast<XSound*>(pCookie)->bankUnloadCallbackFunc(bankID, pInMemoryBankPtr, eLoadResult, memPoolId);
}


void XSound::bankUnloadCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
	AkMemPoolId	memPoolId)
{
	X_UNUSED(pInMemoryBankPtr, memPoolId, eLoadResult);
	
	core::CriticalSection::ScopedLock lock(cs_);

	std::remove_if(banks_.begin(), banks_.end(), [bankID](const Bank&b) {
		return b.bankID == bankID;
	});
}



void XSound::shutDown(void)
{
	X_LOG0("SoundSys", "Shutting Down");
#if X_SUPER == 0
	if (comsSysInit_) {
		Comm::Term();
		comsSysInit_ = false;
	}
#endif // !X_SUPER

	MusicEngine::Term();

	if (AK::SoundEngine::IsInitialized())
	{
		AKRESULT res;

		res = AK::SoundEngine::UnregisterGameObj(GLOBAL_OBJECT_ID);
		if (res != AK_Success) {
			AkResult::Description desc;
			X_ERROR("SoundSys", "Error unregistering global objects. %s", AkResult::ToString(res, desc));
		}

		res = AK::SoundEngine::ClearBanks();
		if (res != AK_Success) {
			AkResult::Description desc;
			X_ERROR("SoundSys", "Error clearing banks. %s", AkResult::ToString(res, desc));
		}


		SoundEngine::Term();
	}

	ioHook_.Term();

	if (IAkStreamMgr::Get()) {
		IAkStreamMgr::Get()->Destroy();
	}

	if (AK::MemoryMgr::IsInitialized())
	{
		MemoryMgr::Term();
	}
}

void XSound::release(void)
{
	X_DELETE(this,g_SoundArena);
}

void XSound::Update(void)
{
	X_PROFILE_BEGIN("SoundUpdate", core::profiler::SubSys::SOUND);

	if (AK::SoundEngine::IsInitialized())
	{
		AkListenerPosition listener;
		listener.OrientationFront.X = 0;
		listener.OrientationFront.Y = 0;
		listener.OrientationFront.Z = 0;
		listener.OrientationTop.X = 0;
		listener.OrientationTop.Y = 0;
		listener.OrientationTop.Z = 1.0f;
		listener.Position = Vec3ToAkVector(listenerTrans_.pos);

		AKRESULT res = SoundEngine::SetListenerPosition(listener);
		if (res != AK_Success) {
			AkResult::Description desc;
			X_ERROR("SoundSys", "Error setting listener pos. %s", AkResult::ToString(res, desc));
		}

		if (vars_.EnableOutputCapture() && !outputCaptureEnabled_)
		{
			AkOSChar const* pFileName = L"audio_capture.wav";

			res = AK::SoundEngine::StartOutputCapture(pFileName);
			if (res != AK_Success) {
				AkResult::Description desc;
				X_ERROR("SoundSys", "Error starting output capture. %s", AkResult::ToString(res, desc));
			}

			outputCaptureEnabled_ = true;
		}
		else if (!vars_.EnableOutputCapture() && outputCaptureEnabled_)
		{
			res = AK::SoundEngine::StopOutputCapture();
			if (res != AK_Success) {
				AkResult::Description desc;
				X_ERROR("SoundSys", "Error stopping output capture. %s", AkResult::ToString(res, desc));
			}

			outputCaptureEnabled_ = false;
		}


		SoundEngine::RenderAudio();
	}
}

void XSound::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(lparam);

	if (event == CoreEvent::CHANGE_FOCUS)
	{
		if (wparam == 1) // activated
		{
			X_LOG2("SoundSys", "Suspending sound system");
			AKRESULT res = AK::SoundEngine::Suspend(false);
			if (res != AK_Success) {
				AkResult::Description desc;
				X_ERROR("SoundSys", "Error suspending. %s", AkResult::ToString(res, desc));
			}
		}
		else
		{
			X_LOG2("SoundSys", "Waking sound system from syspend");

			AKRESULT res = AK::SoundEngine::WakeupFromSuspend();
			if (res != AK_Success) {
				AkResult::Description desc;
				X_ERROR("SoundSys", "Error waking up sound system. %s", AkResult::ToString(res, desc));
			}
			// might need to be called here not sure.
			// SoundEngine::RenderAudio();
		}
	}
}


// Shut up!
void XSound::mute(bool mute)
{
	X_UNUSED(mute);
}

void XSound::setListenPos(const Transformf& trans)
{
	listenerTrans_ = trans;
}

// Volume
void XSound::setMasterVolume(float vol)
{
	// cap it between 0-1
	vol = math<float>::clamp(vol, 0.f, 1.f);
	// turn 0-1 into 0-255
	vol *= 255.f;

	setRTPCValue(GAME_PARAMETERS::MASTERVOLUME, vol);
}


void XSound::setMusicVolume(float vol)
{
	vol = math<float>::clamp(vol, 0.f, 1.f);
	vol *= 255.f;

	setRTPCValue(GAME_PARAMETERS::MUSICVOLUME, vol);
}

void XSound::setVoiceVolume(float vol)
{
	vol = math<float>::clamp(vol, 0.f, 1.f);
	vol *= 255.f;

	setRTPCValue(GAME_PARAMETERS::VOICEVOLUME, vol);
}

void XSound::setSFXVolume(float vol)
{
	vol = math<float>::clamp(vol, 0.f, 1.f);
	vol *= 255.f;

	setRTPCValue(GAME_PARAMETERS::SFXVOLUME, vol);
}

// ----------------------------------------------

uint32_t XSound::getIDFromStr(const char* pStr) const
{
	X_ASSERT(core::strUtil::IsLower(pStr), "must be lower case")(pStr);
	return SoundEngine::GetIDFromString(pStr);
}

uint32_t XSound::getIDFromStr(const wchar_t* pStr) const
{
	X_ASSERT(core::strUtil::IsLower(pStr), "must be lower case")(pStr);
	return SoundEngine::GetIDFromString(pStr);
}


// the id is passed in, so could just pass pointer value in then use that as id.
bool XSound::registerObject(GameObjectID object, const char* pNick)
{
	AKRESULT res = AK::SoundEngine::RegisterGameObj(object, pNick ? pNick : "");
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error registering object. %s", AkResult::ToString(res, desc));
		return false;
	}

	return true;
}

bool XSound::unRegisterObject(GameObjectID object)
{
	AKRESULT res = AK::SoundEngine::UnregisterGameObj(object);
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error un-registering object. %s", AkResult::ToString(res, desc));
		return false;
	}

	return true;
}

void XSound::unRegisterAll(void)
{
	AKRESULT res = AK::SoundEngine::UnregisterAllGameObj();
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error un-registering all object. %s", AkResult::ToString(res, desc));
	}
}


void XSound::setPosition(GameObjectID object, const Transformf& trans)
{
	AKRESULT res = AK::SoundEngine::SetPosition(object, TransToAkPos(trans));
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error setting position of object. %s", AkResult::ToString(res, desc));
	}
}

void XSound::setPosition(GameObjectID* pObjects, const Transformf* pTrans, size_t num)
{
	X_UNUSED(pObjects, pTrans, num);
	X_ASSERT_NOT_IMPLEMENTED();
}

void XSound::stopAll(GameObjectID object)
{
	SoundEngine::StopAll(object);
}

void XSound::postEvent(EventID event, GameObjectID object)
{
	auto playingId = SoundEngine::PostEvent(event, object);
	if (playingId == AK_INVALID_PLAYING_ID)
	{
		X_ERROR("Sound", "Failed to post event %" PRIu32, " object: %" PRIu32, event, object);
	}
	else
	{
		// goaty
		X_LOG0("Sound", "PlayingID: %" PRIu32, playingId);
	}
}

void XSound::setMaterial(GameObjectID object, engine::MaterialSurType::Enum surfaceType)
{
	AkSwitchStateID state;

	static_assert(engine::MaterialSurType::ENUM_COUNT == 26, "More surface types? this needs updating");

	switch (surfaceType)
	{
		case engine::MaterialSurType::BRICK:
			state = AK::SWITCHES::MATERIAL::SWITCH::BRICK;
			break;
		case engine::MaterialSurType::CONCRETE:
			state = AK::SWITCHES::MATERIAL::SWITCH::CONCRETE;
			break;
		case engine::MaterialSurType::CLOTH:
			state = AK::SWITCHES::MATERIAL::SWITCH::CLOTH;
			break;
		case engine::MaterialSurType::CARPET:
			state = AK::SWITCHES::MATERIAL::SWITCH::CARPET;
			break;
		case engine::MaterialSurType::CERAMIC:
			state = AK::SWITCHES::MATERIAL::SWITCH::CERAMIC;
			break;

		case engine::MaterialSurType::DIRT:
			state = AK::SWITCHES::MATERIAL::SWITCH::DIRT;
			break;

		case engine::MaterialSurType::FLESH:
			state = AK::SWITCHES::MATERIAL::SWITCH::FOLIAGE;
			break;
		case engine::MaterialSurType::FOLIAGE:
			state = AK::SWITCHES::MATERIAL::SWITCH::FOLIAGE;
			break;

		case engine::MaterialSurType::GLASS:
			state = AK::SWITCHES::MATERIAL::SWITCH::GLASS;
			break;
		case engine::MaterialSurType::GRASS:
			state = AK::SWITCHES::MATERIAL::SWITCH::GRASS;
			break;
		case engine::MaterialSurType::GRAVEL:
			state = AK::SWITCHES::MATERIAL::SWITCH::GRAVEL;
			break;

		case engine::MaterialSurType::ICE:
			state = AK::SWITCHES::MATERIAL::SWITCH::ICE;
			break;

		case engine::MaterialSurType::METAL:
			state = AK::SWITCHES::MATERIAL::SWITCH::METAL;
			break;
		case engine::MaterialSurType::METAL_THIN:
			state = AK::SWITCHES::MATERIAL::SWITCH::METAL_THIN;
			break;
		case engine::MaterialSurType::METAL_HOLLOW:
			state = AK::SWITCHES::MATERIAL::SWITCH::METAL_HOLLOW;
			break;
		case engine::MaterialSurType::MUD:
			state = AK::SWITCHES::MATERIAL::SWITCH::MUD;
			break;

		case engine::MaterialSurType::PLASTIC:
			state = AK::SWITCHES::MATERIAL::SWITCH::PLASTIC;
			break;
		case engine::MaterialSurType::PAPER:
			state = AK::SWITCHES::MATERIAL::SWITCH::PAPER;
			break;
		case engine::MaterialSurType::PLASTER:
			state = AK::SWITCHES::MATERIAL::SWITCH::PLASTER;
			break;
		case engine::MaterialSurType::ROCK:
			state = AK::SWITCHES::MATERIAL::SWITCH::ROCK;
			break;
		case engine::MaterialSurType::RUBBER:
			state = AK::SWITCHES::MATERIAL::SWITCH::RUBBER;
			break;

		case engine::MaterialSurType::SNOW:
			state = AK::SWITCHES::MATERIAL::SWITCH::SNOW;
			break;
		case engine::MaterialSurType::SAND:
			state = AK::SWITCHES::MATERIAL::SWITCH::SAND;
			break;

		case engine::MaterialSurType::WOOD:
			state = AK::SWITCHES::MATERIAL::SWITCH::WOOD;
			break;
		case engine::MaterialSurType::WATER:
			state = AK::SWITCHES::MATERIAL::SWITCH::WATER;
			break;

		default:
			X_ERROR("SoundSys", "Error unhandled material type: \"%s\"", engine::MaterialSurType::ToString(surfaceType));
			state = AK::SWITCHES::MATERIAL::SWITCH::CONCRETE;
			break;
	}

	AKRESULT res = AK::SoundEngine::SetSwitch(AK::SWITCHES::MATERIAL::GROUP, state, object);
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error when setting material type: \"%s\". %s", engine::MaterialSurType::ToString(surfaceType), AkResult::ToString(res, desc));
	}
}

void XSound::setSwitch(SwitchGroupID group, SwitchStateID state, GameObjectID object)
{
	AKRESULT res = AK::SoundEngine::SetSwitch(group, state, object);
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error when setting switch. group: %" PRIu32 " state: %" PRIu32 " %s", state, group, AkResult::ToString(res, desc));
	}
}

void XSound::setRTPCValue(RtpcID id, RtpcValue val, GameObjectID object,
	core::TimeVal changeDuration, CurveInterpolation::Enum fadeCurve)
{
	AKRESULT res = AK::SoundEngine::SetRTPCValue(id, val, object, ToAkTime(changeDuration), ToAkCurveInterpolation(fadeCurve));
	if (res != AK_Success) {
		AkResult::Description desc;
		X_ERROR("SoundSys", "Error set RTPC failed. %s", AkResult::ToString(res, desc));
	}
}


// ------------------ Commands ----------------------------

// snd_set_rtpc <name> <state> <ObjectId>
void XSound::cmd_SetRtpc(core::IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() < 3) {
		X_WARNING("Console", "snd_set_rtpc <name> <state> <ObjectId>");
		return;
	}

	const char* pName = pArgs->GetArg(1);
	const char* pState = pArgs->GetArg(2);

	// optional ObjectId
	int32_t objectId = 0;
	if (pArgs->GetArgCount() > 3) {
		objectId = core::strUtil::StringToInt<int32_t>(pArgs->GetArg(3));
	}

	// TODO
	X_UNUSED(pName);
	X_UNUSED(pState);
	X_UNUSED(objectId);
}

// snd_set_switchstate <name> <state> <ObjectId>
void XSound::cmd_SetSwitchState(core::IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() < 3) {
		X_WARNING("Console", "snd_set_switchstate <name> <state> <ObjectId>");
		return;
	}

	const char* pName = pArgs->GetArg(1);
	const char* pState = pArgs->GetArg(2);

	// optional ObjectId
	AkGameObjectID objectId = GLOBAL_OBJECT_ID;
	if (pArgs->GetArgCount() > 3) {
		objectId = core::strUtil::StringToInt<AkGameObjectID>(pArgs->GetArg(3));
	}

	// TODO
	X_UNUSED(pName);
	X_UNUSED(pState);
	X_UNUSED(objectId);
}

// snd_post_event <eventName> <ObjectId>
void XSound::cmd_PostEvent(core::IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() < 2) {
		X_WARNING("Console", "snd_post_event <eventName> <ObjectId>");
		return;
	}

	const char* pEventName = pArgs->GetArg(1);

	// optional ObjectId
	AkGameObjectID objectId = GLOBAL_OBJECT_ID;
	if (pArgs->GetArgCount() > 2) {
		objectId = core::strUtil::StringToInt<AkGameObjectID>(pArgs->GetArg(2));
	}

	auto eventId = SoundEngine::GetIDFromString(pEventName);
	X_LOG1("Sound", "snd_post_event: id: %" PRIu32, eventId);

	postEvent(eventId, objectId);
}


// snd_stop_event <eventName> <ObjectId>
void XSound::cmd_StopEvent(core::IConsoleCmdArgs* pArgs)
{
	if (pArgs->GetArgCount() < 2) {
		X_WARNING("Console", "snd_stop_event <eventName> <ObjectId>");
		return;
	}

	const char* pEventName = pArgs->GetArg(1);

	// optional ObjectId
	AkGameObjectID objectId = GLOBAL_OBJECT_ID;
	if (pArgs->GetArgCount() > 2) {
		objectId = core::strUtil::StringToInt<AkGameObjectID>(pArgs->GetArg(2));
	}

	// TODO
	X_UNUSED(pEventName);
	X_UNUSED(objectId);
}


// snd_stop_all_event <ObjectId>
void XSound::cmd_StopAllEvent(core::IConsoleCmdArgs* pArgs)
{
	// optional ObjectId
	AkGameObjectID objectId = GLOBAL_OBJECT_ID;
	if (pArgs->GetArgCount() > 1) {
		objectId = core::strUtil::StringToInt<AkGameObjectID>(pArgs->GetArg(1));
	}

	// TODO
	X_UNUSED(objectId);
}

void XSound::cmd_ListBanks(core::IConsoleCmdArgs* pArgs)
{
	const char* pSearchString = nullptr;
	if (pArgs->GetArgCount() > 1) {
		pSearchString = pArgs->GetArg(1);
	}

	listBanks(pSearchString);
}


X_NAMESPACE_END