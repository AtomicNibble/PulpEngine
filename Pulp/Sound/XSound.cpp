#include "stdafx.h"
#include "XSound.h"

#include <Math\XMatrixAlgo.h>

#include "ScriptBinds\ScriptBinds_sound.h"

// id's
#include "IDs\Wwise_IDs.h"

#include <IConsole.h>
#include <I3DEngine.h>
#include <IPrimitiveContext.h>
#include <IPhysics.h>
#include <IFrameData.h>
#include <IFont.h>

X_DISABLE_WARNING(4505)

// Tools common
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAssert.h>

// Comms
#include <AK/Comm/AkCommunication.h>

X_ENABLE_WARNING(4505)

#if X_DEBUG

#if !X_ENABLE_SOUND_COMS
#error must have sound coms for debug 
#endif // !X_ENABLE_SOUND_COMS

#define WWISE_LIB "Debug"
#elif X_ENABLE_SOUND_COMS
#define WWISE_LIB "Profile" // Profile is 
#else
#define WWISE_LIB "Release"
#endif

#define X_LINK_LIB_WWISE(name) X_LINK_LIB(WWISE_LIB "/" name)

// link all plugins
#define PLUGIN_All 1

#if PLUGIN_All == 0
#define PLUGIN_Codec 1
#define PLUGIN_Effect 1
#define PLUGIN_Source 1
#define PLUGIN_Auro 0
#else
#define PLUGIN_Codec 1
#define PLUGIN_Effect 1
#define PLUGIN_Source 1
#define PLUGIN_Auro 0
#endif // !PLUGIN_All

// link the libs..

// engine
X_LINK_LIB_WWISE("AkMusicEngine");
X_LINK_LIB_WWISE("AkSoundEngine");
X_LINK_LIB_WWISE("AkMusicEngine");

// managers
X_LINK_LIB_WWISE("AkMemoryMgr");
X_LINK_LIB_WWISE("AkStreamMgr");

// decoeer / source?
#if PLUGIN_Codec
#include <AK/Plugin/AkVorbisDecoderFactory.h>
X_LINK_LIB_WWISE("AkVorbisDecoder");
#endif // !PLUGIN_Codec

// source
#include <AK/Plugin/AkAudioInputSourceFactory.h>
X_LINK_LIB_WWISE("AkAudioInputSource");
// #include <AK/Plugin/AkSilenceSourceFactory.h>
// X_LINK_LIB_WWISE("AkSilenceSource");
// #include <AK/Plugin/AkSineSourceFactory.h>
// X_LINK_LIB_WWISE("AkSineSource");
// #include <AK/Plugin/AkSoundSeedWindSourceFactory.h>
// X_LINK_LIB_WWISE("AkSoundSeedWindSource");
// #include <AK/Plugin/AkSoundSeedWooshSourceFactory.h>
// X_LINK_LIB_WWISE("AkSoundSeedWooshSource");
// #include <AK/Plugin/AkSynthOneSourceFactory.h>
// X_LINK_LIB_WWISE("AkSynthOneSource");
// #include <AK/Plugin/AkToneSourceFactory.h>
// X_LINK_LIB_WWISE("AkToneSource");

// fx
// TODO: add includes if enable.
// X_LINK_LIB_WWISE("AkCompressorFX");
// X_LINK_LIB_WWISE("AkDelayFX");
// X_LINK_LIB_WWISE("AkExpanderFX");
// X_LINK_LIB_WWISE("AkFlangerFX");
// X_LINK_LIB_WWISE("AkGainFX");
// X_LINK_LIB_WWISE("AkGuitarDistortionFX");
// X_LINK_LIB_WWISE("AkHarmonizerFX");
// X_LINK_LIB_WWISE("AkMatrixReverbFX");
// X_LINK_LIB_WWISE("AkMeterFX");
// X_LINK_LIB_WWISE("AkParametricEQFX");
// X_LINK_LIB_WWISE("AkPeakLimiterFX");
// X_LINK_LIB_WWISE("AkPitchShifterFX");
// X_LINK_LIB_WWISE("AkRoomVerbFX");
// X_LINK_LIB_WWISE("AkSoundSeedImpactFX");
// X_LINK_LIB_WWISE("AkStereoDelayFX");
// X_LINK_LIB_WWISE("AkTimeStretchFX");
// X_LINK_LIB_WWISE("AkTremoloFX");


// X_LINK_LIB_WWISE("McDSPFutzBoxFX");
// X_LINK_LIB_WWISE("McDSPLimiterFX");

#if PLUGIN_Auro
#include <AK/Plugin/AuroHeadphoneFXFactory.h>
X_LINK_LIB_WWISE("AuroHeadphoneFX");
#endif // !PLUGIN_Auro


// new?
#if 0
#include <AK/Plugin/AuroHeadphoneFXFactory.h>
X_LINK_LIB_WWISE("AkMotionGeneratorSource");
#include <AK/Plugin/AkMotionSinkFactory.h>
X_LINK_LIB_WWISE("AkMotionSink");
#include <AK/Plugin/AkRecorderFXFactory.h>
X_LINK_LIB_WWISE("AkRecorderFX");
#endif

// fx2
#if 0 
X_LINK_LIB_WWISE("iZTrashBoxModelerFX");
X_LINK_LIB_WWISE("iZTrashDelayFX");
X_LINK_LIB_WWISE("iZTrashMultibandDistortionFX");
X_LINK_LIB_WWISE("iZHybridReverbFX");
X_LINK_LIB_WWISE("iZTrashDynamicsFX");
X_LINK_LIB_WWISE("iZTrashDistortionFX");
X_LINK_LIB_WWISE("iZTrashFiltersFX");
#endif

// misc
#if PLUGIN_Auro
#include <AK/Plugin/AuroPannerMixerFactory.h>
X_LINK_LIB_WWISE("AuroPannerMixer");
#endif // !PLUGIN_Auro

// platform
X_LINK_LIB("dinput8");
X_LINK_LIB("dxguid");

#if X_ENABLE_SOUND_COMS
X_LINK_LIB("ws2_32"); // used for winsock
X_LINK_LIB_WWISE("CommunicationCentral");
#endif // X_ENABLE_SOUND_COMS

using namespace AK;

using namespace core::string_view_literals;

X_NAMESPACE_BEGIN(sound)

namespace
{
    void akAssertHook(const char* pszExpression, const char* pszFileName, int lineNumber)
    {
#if X_ENABLE_ASSERTIONS
        core::SourceInfo sourceInfo(pszFileName, lineNumber, "", "");
        core::Assert(sourceInfo, "Assertion \"%s\" failed.", pszExpression)
            .Variable("FileName", pszFileName)
            .Variable("LineNumber", lineNumber);

#else
        X_ERROR("SoundSys", "Sound system threw a assert: Exp: \"%s\" file: \"%s\" line: \"%s\"",
            pszExpression, pszFileName, lineNumber);
#endif

        X_BREAKPOINT;
    }

    X_INLINE AkGameObjectID GameObjHandleToAKObject(SndObjectHandle object)
    {
        static_assert(sizeof(SndObjectHandle) <= sizeof(AkGameObjectID), "can't represent type");
        return reinterpret_cast<AkGameObjectID>(&object);
    }

    X_INLINE AkGameObjectID SoundObjToAKObject(SoundObject* pObject)
    {
        static_assert(sizeof(SoundObject*) <= sizeof(AkGameObjectID), "can't represent type");
        return reinterpret_cast<AkGameObjectID>(pObject);
    }

    X_INLINE SndObjectHandle SoundObjToObjHandle(SoundObject* pObject)
    {
        static_assert(sizeof(SoundObject*) <= sizeof(SndObjectHandle), "can't represent type");
        return reinterpret_cast<SndObjectHandle>(pObject);
    }

    X_INLINE SoundObject* SoundHandleToObject(SndObjectHandle object)
    {
        static_assert(sizeof(SoundObject*) <= sizeof(SndObjectHandle), "can't represent type");
        return reinterpret_cast<SoundObject*>(object);
    }

    X_INLINE SoundObject* AKObjectToObject(AkGameObjectID object)
    {
        static_assert(sizeof(SoundObject*) <= sizeof(AkGameObjectID), "can't represent type");
        return reinterpret_cast<SoundObject*>(object);
    }

} // namespace

static_assert(sizeof(SndObjectHandle) == sizeof(AkGameObjectID), "Handle size mismatch");
static_assert(sizeof(PlayingID) == sizeof(AkPlayingID), "PlayingID size mismatch");

static_assert(GLOBAL_OBJECT_ID == static_cast<SndObjectHandle>(2), "Should be 2 yo");
static_assert(LISTNER_OBJECT_ID == static_cast<SndObjectHandle>(1), "Should be 1 yo");
static_assert(INVALID_OBJECT_ID == AK_INVALID_GAME_OBJECT, "Invalid id incorrect");

XSound::XSound(core::MemoryArenaBase* arena) :
    arena_(arena),
    pPrimCon_(nullptr),
    pScene_(nullptr),
    banks_(arena_),
    packages_(arena),
    objectPool_(arena_, sizeof(SoundObject), X_ALIGN_OF(SoundObject), "SoundObjPool"),
    objects_(arena_),
    culledObjects_(arena_),
    occlusion_(arena_),
    comsSysInit_(false),
    outputCaptureEnabled_(false),
    bankSignal_(true),
    suspended_(false),
    pScriptBinds_(nullptr)
{
    objects_.reserve(128);

    ttSetLockName(gEnv->ctx, &cs_, "SoundLock");
    ttSetLockName(gEnv->ctx, &memStreamCS_, "SoundMemStreamLock");
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

    ADD_COMMAND_MEMBER("sndSetRtpc", this, XSound, &XSound::cmd_SetRtpc, core::VarFlag::SOUND,
        "Set a audio RTPC value. <name> <state> <ObjectId>");
    ADD_COMMAND_MEMBER("sndSetWwitchstate", this, XSound, &XSound::cmd_SetSwitchState, core::VarFlag::SOUND,
        "Set a audio Switch State. <name> <state> <ObjectId>");

    ADD_COMMAND_MEMBER("sndPostEvent", this, XSound, &XSound::cmd_PostEvent, core::VarFlag::SOUND,
        "Post a audio event. <eventName> <ObjectId>");
    ADD_COMMAND_MEMBER("sndStopEvent", this, XSound, &XSound::cmd_StopEvent, core::VarFlag::SOUND,
        "Stop a audio event on a object. <eventName> <ObjectId>");

    ADD_COMMAND_MEMBER("sndStopEventAll", this, XSound, &XSound::cmd_StopAllEvent, core::VarFlag::SOUND,
        "Stop all audio events for a object. <ObjectId>");

    ADD_COMMAND_MEMBER("listSndBanks", this, XSound, &XSound::cmd_ListBanks, core::VarFlag::SOUND,
        "List all the loaded sound banks");
}

void XSound::registerScriptBinds(void)
{
    auto* pScriptSys = gEnv->pScriptSys;

    pScriptBinds_ = X_NEW(ScriptBinds_Sound, arena_, "SoundScriptBinds")(pScriptSys, this);

    pScriptBinds_->bind();
}

bool XSound::init(void)
{
    X_LOG0("SoundSys", "Starting");
    X_PROFILE_NO_HISTORY_BEGIN("SoundInit", core::profiler::SubSys::SOUND);
    ttZone(gEnv->ctx, "(Sound) Init");

    gEnv->pCore->GetCoreEventDispatcher()->RegisterListener(this);

    // TODO: call from somewhere.
    registerScriptBinds();

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
    // Memory is hooked in Allocators.cpp
    if (MemoryMgr::Init(&memSettings) != AK_Success) {
        X_ERROR("SoundSys", "Could not create the memory manager.");
        return false;
    }

    // Create and initialise an instance of the default stream manager.
    stmSettings.uMemorySize = vars_.StreamManagerMemoryPoolBytes();
    if (!StreamMgr::Create(stmSettings)) {
        X_ERROR("SoundSys", "Could not create the Stream Manager");
        return false;
    }

    // Create an IO device.
    deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
    deviceSettings.uIOMemorySize = vars_.StreamDeviceMemoryPoolBytes();
    if (ioHook_.Init(deviceSettings, true) != AK_Success) {
        X_ERROR("SoundSys", "Cannot create streaming I/O device");
        return false;
    }

    // Initialize sound engine.
    l_InitSettings.pfnAssertHook = akAssertHook;
    l_InitSettings.uDefaultPoolSize = vars_.SoundEngineDefaultMemoryPoolBytes();
    l_InitSettings.uCommandQueueSize = vars_.CommandQueueMemoryPoolBytes();
    l_InitSettings.uMonitorPoolSize = vars_.MonitorMemoryPoolBytes();
    l_InitSettings.uMonitorQueuePoolSize = vars_.MonitorQueueMemoryPoolBytes();

    l_platInitSetings.uLEngineDefaultPoolSize = vars_.SoundEngineLowerDefaultMemoryPoolBytes();
    
    l_platInitSetings.eAudioAPI = AkAudioAPI::AkAPI_Default;
    {
        auto outputDevice = gEnv->pCore->GetCommandLineArg("snd_output_device"_sv);
        if (outputDevice) {
            if (core::strUtil::IsEqualCaseInsen(outputDevice, "xaudio2"_sv)) {
                l_platInitSetings.eAudioAPI = AkAudioAPI::AkAPI_XAudio2;
                X_LOG1("SoundSys", "using output device: XAudio2");
            }
            else if (core::strUtil::IsEqualCaseInsen(outputDevice, "directsound"_sv)) {
                l_platInitSetings.eAudioAPI = AkAudioAPI::AkAPI_DirectSound;
                X_LOG1("SoundSys", "using output device: directsound (DirectX Sound)");
            }
            else if (core::strUtil::IsEqualCaseInsen(outputDevice, "wasapi"_sv)) {
                l_platInitSetings.eAudioAPI = AkAudioAPI::AkAPI_Wasapi;
                X_LOG1("SoundSys", "using output device: Wasapi (Windows Audio Session)");
            }
            else if (core::strUtil::IsEqualCaseInsen(outputDevice, "none"_sv)) {
                l_platInitSetings.eAudioAPI = AkAudioAPI::AkAPI_Default;
                X_LOG1("SoundSys", "using output device: none (no sound)");
            }
            else {
                X_ERROR("SoundSys", "Unknown output device \"%.*s\" using default instead. valid options: "
                    "xaudio2, directsound, wasapi, none", outputDevice.length(), outputDevice.data());
            }
        }
    }

    if (SoundEngine::Init(&l_InitSettings, &l_platInitSetings) != AK_Success) {
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
    if (MusicEngine::Init(&musicInit) != AK_Success) {
        X_ERROR("SoundSys", "Cannot initialize music engine");
        return false;
    }

#if X_ENABLE_SOUND_COMS

    if (vars_.EnableComs()) {
        // Initialize communication.
        AkCommSettings settingsComm;
        AK::Comm::GetDefaultInitSettings(settingsComm);
        AKPLATFORM::SafeStrCpy(settingsComm.szAppNetworkName, X_ENGINE_NAME, AK_COMM_SETTINGS_MAX_STRING_SIZE);
        if (AK::Comm::Init(settingsComm) != AK_Success) {
            X_ERROR("SoundSys", "Cannot initialize Wwise communication");
            return false;
        }

        comsSysInit_ = true;
    }
#endif // !X_ENABLE_SOUND_COMS


    SetAudioInputCallbacks(executeCallback_s, getFormatCallback_s, nullptr);

    vars_.applyVolume();

    if (!registerBaseObjects()) {
        X_ERROR("SoundSys", "Failed to register base objects");
        return false;
    }

    // Mount packages, this is a blocking IO call currently.
    // If I decide to never store banks in packages, I could make it async.
    {
        X_PROFILE_NO_HISTORY_BEGIN("SoundPckLoad", core::profiler::SubSys::SOUND);
        ttZone(gEnv->ctx, "(Sound) LoadPacks");

        if (!loadPackage("streamed.pck")) {
            return false;
        }
    }

    // dispatch async loads for base banks.
    loadBank("Init.bnk");
    loadBank("Events.bnk");

    // temp
    loadBank("PlayerSounds.bnk");
    loadBank("Ambient.bnk");
    loadBank("Weapons.bnk");
    loadBank("SFX.bnk");
    loadBank("Music.bnk");

    return true;
}

bool XSound::asyncInitFinalize(void)
{
    bool loaded = true;

    for (auto& bank : banks_) {
		auto ok = waitForBankLoad(bank);

		if (!ok)
		{
			X_ERROR("SoundSys", "Failed to load bank: \"%s\"", bank.name.c_str());
		}

		loaded &= ok;
    }

    pPrimCon_ = gEnv->p3DEngine->getPrimContext(engine::PrimContext::SOUND);
    if (!pPrimCon_) {
        X_ERROR("SoundSys", "Failed to get prim context");
        return false;
    }

    return loaded;
}

void XSound::shutDown(void)
{
    X_LOG0("SoundSys", "Shutting Down");

    gEnv->pCore->GetCoreEventDispatcher()->RemoveListener(this);

    if (pScriptBinds_) {
        X_DELETE(pScriptBinds_, arena_);
    }

#if X_ENABLE_SOUND_COMS
    if (comsSysInit_) {
        Comm::Term();
        comsSysInit_ = false;
    }
#endif // !X_ENABLE_SOUND_COMS

    freeDangling();

    MusicEngine::Term();

    if (AK::SoundEngine::IsInitialized()) {
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

    // unloads packages for us.
    ioHook_.Term();

    if (IAkStreamMgr::Get()) {
        IAkStreamMgr::Get()->Destroy();
    }

    if (AK::MemoryMgr::IsInitialized()) {
        MemoryMgr::Term();
    }
}

void XSound::release(void)
{
    X_DELETE(this, g_SoundArena);
}

Vec2f XSound::drawDebug(engine::IPrimitiveContext* pPrim, Vec2f pos) const
{
    if (!vars_.drawDebug()) {
        return Vec2f();
    }

    const float padding = 16.f;

    font::TextDrawContext ctx;
    ctx.col = Col_Dimgray;
    ctx.pFont = gEnv->pFontSys->getDefault();
    ctx.size = Vec2f(16.f, 16.f);

    Vec2f size(200, 80);
    Rectf r(pos, pos + size);

    pPrim->drawQuad(r, Color(0.1f, 0.1f, 0.1f, 0.8f));

    core::StackString<64> txt;
    txt.setFmt("Snd Obj: ^6%" PRIuS "^~\n", objects_.size());
    txt.appendFmt("Culled: ^6%" PRIuS, culledObjects_.size());

    pPrim->drawText(Vec3f(r.getUpperLeft() + Vec2f(padding, padding)), ctx, txt.begin(), txt.end());

    return size;
}

void XSound::update(core::FrameData& frame)
{
    X_PROFILE_BEGIN("SoundUpdate", core::profiler::SubSys::SOUND);
    ttZone(gEnv->ctx, "(Sound) Sys Update");

    if (AK::SoundEngine::IsInitialized()) {
        if (vars_.EnableCulling()) {
            cullObjects();
        }

        core::TimeVal sinceLastOcclude = frame.timeInfo.startTimeReal - lastOcclusionUpdate_;
        core::TimeVal refreshInterval(vars_.OcclusionRefreshRate());
        if (sinceLastOcclude > refreshInterval) {
            lastOcclusionUpdate_ = frame.timeInfo.startTimeReal;

            performOcclusionChecks();
        }

        AkListenerPosition listenerPos = TransToAkPos(listenerTrans_);

        AKRESULT res;
        res = SoundEngine::SetPosition(LISTNER_OBJECT_ID, listenerPos);
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error setting listener pos. %s", AkResult::ToString(res, desc));
        }

        if (vars_.EnableOutputCapture() && !outputCaptureEnabled_) {
            AkOSChar const* pFileName = L"audio_capture.wav";

            res = AK::SoundEngine::StartOutputCapture(pFileName);
            if (res != AK_Success) {
                AkResult::Description desc;
                X_ERROR("SoundSys", "Error starting output capture. %s", AkResult::ToString(res, desc));
            }

            outputCaptureEnabled_ = true;
        }
        else if (!vars_.EnableOutputCapture() && outputCaptureEnabled_) {
            res = AK::SoundEngine::StopOutputCapture();
            if (res != AK_Success) {
                AkResult::Description desc;
                X_ERROR("SoundSys", "Error stopping output capture. %s", AkResult::ToString(res, desc));
            }

            outputCaptureEnabled_ = false;
        }

        SoundEngine::RenderAudio();
    }

    if (vars_.EnableDebugRender() > 0) {
        drawDebug();
    }
}

void XSound::setPhysicsScene(physics::IScene* pScene)
{
    pScene_ = pScene;
}

void XSound::drawDebug(void) const
{
    if (!pPrimCon_) {
        return;
    }

    if (vars_.debugObjectScale() < 0.01f) {
        return;
    }

    Sphere sphere;
    sphere.setRadius(vars_.debugObjectScale());

    Color8u sphereCol = Col_Blue;
    Color8u lineCol = Col_Darksalmon;
    Color8u lineColOcc = Col_Red;
    Color8u lineColOccVis = Col_Green;

    pPrimCon_->setDepthTest(true);

    auto listnerPos = listenerTrans_.pos;
    listnerPos.z -= 1.f; // make the line not point directly at us.

    const float cullDistance = vars_.RegisteredCullDistance();

    // prevent using really high quality lods.
    const int32_t minLod = 3;

    const bool drawText = vars_.EnableDebugRender() > 1;
    const float textSize = vars_.debugTextSize();
    font::TextDrawContext con;
    con.effectId = 0;
    con.col = Col_White;
    con.flags.Set(font::DrawTextFlag::CENTER);
    con.flags.Set(font::DrawTextFlag::CENTER_VER);
    con.pFont = gEnv->pFontSys->getDefault();
    con.size = Vec2f(textSize, textSize);

    core::CriticalSection::ScopedLock lock(cs_);
    for (auto* pObject : objects_) {
        const auto& trans = pObject->trans;
        sphere.setCenter(trans.pos);

        float distance = listnerPos.distance(trans.pos);
        if (distance > cullDistance) {
            continue;
        }

        int32_t lodIdx = static_cast<int32_t>(distance / 500.f);
        lodIdx = math<int32_t>::clamp(lodIdx, minLod, engine::IPrimitiveContext::SHAPE_NUM_LOD - 1);

        X_ASSERT(lodIdx >= 0, "invalid index")(lodIdx);

        if (drawText) {
            const Vec3f& eye = trans.pos;
            const Vec3f& center = listnerPos;

            Matrix33f mat;
            MatrixLookAtRH(&mat, center, eye, Vec3f::zAxis());
			mat.transpose();
            mat.rotate(Vec3f::xAxis(), ::toRadians(180.f)); // flip text.

            core::StackString256 txt;
            txt.setFmt("\"%s\" Occ: %s Evt: %i", pObject->debugName.c_str(), OcclusionType::ToString(pObject->occType), pObject->activeEvents);

            pPrimCon_->drawText(trans.pos + Vec3f(0, 0, sphere.radius() + 6.f), mat, con, txt.begin(), txt.end());
        }

        pPrimCon_->drawSphere(sphere, sphereCol, true, lodIdx);

        if (pObject->flags.IsSet(SoundFlag::Occlusion)) {
            pPrimCon_->drawLine(trans.pos, listnerPos, pObject->flags.IsSet(SoundFlag::Occluded) ? lineColOcc : lineColOccVis);
        }
        else {
            pPrimCon_->drawLine(trans.pos, listnerPos, lineCol);
        }
    }
}

void XSound::cullObjects(void)
{
    // so we can only cull objects that are not playing anything.
    // and as soon as we want to play something on it we need to re-register it.
    ttZone(gEnv->ctx, "(Sound) Cull");

    // work out what objects we can register.
    core::CriticalSection::ScopedLock lock(cs_);

    const size_t culledNum = culledObjects_.size();
    const float cullDistance = vars_.RegisteredCullDistance();
    const auto listenerPos = listenerTrans_.pos;

    for (size_t i = 0; i < objects_.size(); i++) {
        auto* pObject = objects_[i];
        if (pObject->activeEvents) {
            continue;
        }

        float distance = pObject->trans.pos.distance(listenerPos);
        if (distance > cullDistance) {
            unregisterObjectSndEngine(pObject);
        }
    }

    if (culledNum < culledObjects_.size()) {
        X_LOG0("SoundSys", "Un-Registered %" PRIuS " object(s)", culledObjects_.size() - culledNum);
    }
}

void XSound::performOcclusionChecks(void)
{
    if (occlusion_.isEmpty()) {
        return;
    }

    ttZone(gEnv->ctx, "(Sound) Occlusion");

    physics::IScene* pScene = pScene_;
    if (!pScene) {
        X_WARNING("SoundSys", "Can't occlude %" PRIuS " emitters, no physical world", occlusion_.size());
        return;
    }

    // TODO: sort these so can do in batches.

    for (auto* pObject : occlusion_) {
        X_ASSERT(pObject->occType != OcclusionType::None, "Object don't have occlusion type set")();

        if (pObject->occType == OcclusionType::SingleRay) {

            const auto& listenerPos = listenerTrans_.pos;

            const Vec3f target = pObject->trans.pos;
            const Vec3f& start = listenerPos;

            Vec3f dir = target - start;
            dir.normalize();

            float distance = start.distance(target);

            physics::ScopedLock lock(pScene, physics::LockAccess::Read);

            physics::RaycastBuffer hit;
            if (pScene->raycast(start, dir, distance, hit, physics::DEFAULT_HIT_FLAGS, physics::QueryFlag::STATIC)) {
                pObject->flags.Set(SoundFlag::Occluded);

                AK::SoundEngine::SetObjectObstructionAndOcclusion(SoundObjToAKObject(pObject), LISTNER_OBJECT_ID, 0.f, 0.5f);
            }
            else {
                pObject->flags.Remove(SoundFlag::Occluded);

                AK::SoundEngine::SetObjectObstructionAndOcclusion(SoundObjToAKObject(pObject), LISTNER_OBJECT_ID, 0.f, 0.f);
            }
        }
        else {
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }
}

void XSound::registerObjectSndEngine(SoundObject* pObject)
{
    X_ASSERT(!pObject->flags.IsSet(SoundFlag::Registered), "Double register")();

    pObject->flags.Set(SoundFlag::Registered);

#if X_SOUND_ENABLE_DEBUG_NAMES
    AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject), pObject->debugName.c_str());
#else
    AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject));
#endif
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error registering object. %s", AkResult::ToString(res, desc));
    }

    if (pObject->flags.IsSet(SoundFlag::Position)) {
        res = AK::SoundEngine::SetPosition(SoundObjToAKObject(pObject), TransToAkPos(pObject->trans));
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error setting position of object. %s", AkResult::ToString(res, desc));
        }
    }

    culledObjects_.remove(pObject);
    objects_.push_back(pObject);
    if (pObject->flags.IsSet(SoundFlag::Occlusion)) {
        AK::SoundEngine::SetObjectObstructionAndOcclusion(SoundObjToAKObject(pObject), LISTNER_OBJECT_ID, 0.f, 0.f);

        occlusion_.push_back(pObject);
    }
}

void XSound::unregisterObjectSndEngine(SoundObject* pObject)
{
    X_ASSERT(pObject->flags.IsSet(SoundFlag::Registered), "Double un-register")();

    pObject->flags.Remove(SoundFlag::Registered);

    AKRESULT res = AK::SoundEngine::UnregisterGameObj(SoundObjToAKObject(pObject));
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error un-registering object. %s", AkResult::ToString(res, desc));
    }

    objects_.remove(pObject);
    culledObjects_.push_back(pObject);
    if (pObject->flags.IsSet(SoundFlag::Occlusion)) {
        occlusion_.remove(pObject);
    }
}

// ---------------------------------------------

// Shut up!
void XSound::mute(bool mute)
{
    if (!AK::SoundEngine::IsInitialized()) {
        X_WARNING("SoundSys", "Mute called when sound system not init");
        return;
    }

    if (suspended_ == mute) {
        return;
    }

    if (mute)
    {
        X_LOG2("SoundSys", "Suspending sound system");
        AKRESULT res = AK::SoundEngine::Suspend(true);
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error suspending. %s", AkResult::ToString(res, desc));
        }
    }
    else
    {
        X_LOG2("SoundSys", "Waking up sound system");

        AKRESULT res = AK::SoundEngine::WakeupFromSuspend();
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error waking up sound system. %s", AkResult::ToString(res, desc));
        }
    }

    suspended_ = mute;
}

void XSound::setListenPos(const Transformf& trans)
{
    listenerTrans_ = trans;
}

// Volume - these change SoundEngine, just done this way so vars reflect actual values.
void XSound::setMasterVolume(float vol)
{
    vars_.setMasterVolume(vol);
}

void XSound::setMusicVolume(float vol)
{
    vars_.setMusicVolume(vol);
}

void XSound::setVoiceVolume(float vol)
{
    vars_.setVoiceVolume(vol);
}

void XSound::setSFXVolume(float vol)
{
    vars_.setSFXVolume(vol);
}

// ----------------------------------------------

uint32_t XSound::getIDFromStr(core::string_view str) const
{
    X_ASSERT(core::strUtil::IsLower(str.begin(), str.end()), "must be lower case")();
    return core::Hash::Fnv1Hash(str.data(), str.length());
}

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

// ----------------------------------------------

// the id is passed in, so could just pass pointer value in then use that as id.
SndObjectHandle XSound::registerObject(X_SOUND_DEBUG_NAME(const char* pNick))
{
    core::CriticalSection::ScopedLock lock(cs_);

    SoundObject* pObject = allocObject();
    pObject->flags.Set(SoundFlag::Registered);

#if X_SOUND_ENABLE_DEBUG_NAMES
    if (pNick) {
        pObject->debugName = pNick;
    }
    AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject), pObject->debugName.c_str());
#else
    AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject));
#endif
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error registering object. %s", AkResult::ToString(res, desc));
        freeObject(pObject);
        return INVALID_OBJECT_ID;
    }

    objects_.push_back(pObject);
    return SoundObjToObjHandle(pObject);
}

SndObjectHandle XSound::registerObject(const Transformf& trans X_SOUND_DEBUG_NAME_COM(const char* pNick))
{
    SoundObject* pObject = allocObject();
#if X_SOUND_ENABLE_DEBUG_NAMES
    if (pNick) {
        pObject->debugName = pNick;
    }
#endif

    // we don't make objects very far away active by default.
    // if you play something on these distant objects, they will get registered automatically.
    float distance = listenerTrans_.pos.distance(trans.pos);
    if (distance > vars_.RegisteredCullDistance()) {
        culledObjects_.push_back(pObject);
    }
    else {
        pObject->flags.Set(SoundFlag::Registered);

#if X_SOUND_ENABLE_DEBUG_NAMES
        AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject), pObject->debugName.c_str());
#else
        AKRESULT res = AK::SoundEngine::RegisterGameObj(SoundObjToAKObject(pObject));
#endif
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error registering object. %s", AkResult::ToString(res, desc));
            freeObject(pObject);
            return INVALID_OBJECT_ID;
        }

        setPosition(SoundObjToObjHandle(pObject), trans);

        objects_.push_back(pObject);
    }

    return SoundObjToObjHandle(pObject);
}

bool XSound::unRegisterObject(SndObjectHandle object)
{
    core::CriticalSection::ScopedLock lock(cs_);

    SoundObject* pSound = SoundHandleToObject(object);
    if (pSound->activeEvents) {
        AK::SoundEngine::StopAll(object);
        AK::SoundEngine::CancelEventCallbackGameObject(object); // When we stop all, it might trigger callbacks (which i don't want)
    }

    AKRESULT res = AK::SoundEngine::UnregisterGameObj(object);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error un-registering object. %s", AkResult::ToString(res, desc));
        return false;
    }

    if (pSound->flags.IsSet(SoundFlag::Registered)) {
        objects_.remove(pSound);
        if (pSound->flags.IsSet(SoundFlag::Occlusion)) {
            occlusion_.remove(pSound);
        }
    }
    else {
        culledObjects_.remove(pSound);
    }

    freeObject(pSound);
    return true;
}

bool XSound::registerBaseObjects(void)
{
    AKRESULT res;

    // Register the main listener.
    res = AK::SoundEngine::RegisterGameObj(LISTNER_OBJECT_ID, "Default Listener");
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error creating listener object. %s", AkResult::ToString(res, desc));
        return false;
    }

    res = AK::SoundEngine::SetDefaultListeners(&LISTNER_OBJECT_ID, 1);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error setting default listener. %s", AkResult::ToString(res, desc));
        return false;
    }

    // register a object for stuff with no position.
    res = AK::SoundEngine::RegisterGameObj(GLOBAL_OBJECT_ID, "GlobalObject");
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error creating global object. %s", AkResult::ToString(res, desc));
        return false;
    }

    return true;
}

void XSound::unRegisterAll(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    AKRESULT res = AK::SoundEngine::UnregisterAllGameObj();
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error un-registering all object. %s", AkResult::ToString(res, desc));
    }

    for (auto* pObject : objects_) {
        objectPool_.free(pObject);
    }

    objects_.clear();
    culledObjects_.clear();
    occlusion_.clear();

    if (!registerBaseObjects()) {
        X_ERROR("SoundSys", "Failed to re-register base objects");
    }
}

void XSound::freeDangling(void)
{
    {
        core::CriticalSection::ScopedLock lock(cs_);

        if (objects_.isNotEmpty()) {
            X_WARNING("Sound", "Cleaning up %" PRIuS " dangling sound objects", objects_.size());
        }
    }

    {
        core::CriticalSection::ScopedLock lock(memStreamCS_);

        if (memoryInputStreams_.isNotEmpty()) {
            X_WARNING("Sound", "Cleaning up %" PRIuS " dangling memory streams", memoryInputStreams_.size());

            for (auto& mis : memoryInputStreams_) {
                AK::SoundEngine::StopPlayingID(mis.playingID);
            }
        }

        memoryInputStreams_.clear();
    }

    unRegisterAll();
}

SoundObject* XSound::allocObject(void)
{
    return objectPool_.allocate();
}

void XSound::freeObject(SoundObject* pObject)
{
    objectPool_.free(pObject);
}

SoundObject* XSound::findObjectForNick(core::string_view name)
{
    if (name.empty()) {
        return nullptr;
    }

    core::CriticalSection::ScopedLock lock(cs_);

    for (auto* pObject : objects_) {
        if (pObject->debugName.isNotEmpty() && core::strUtil::IsEqual(core::string_view(pObject->debugName), name)) {
            return pObject;
        }
    }

    for (auto* pObject : culledObjects_) {
        if (pObject->debugName.isNotEmpty() && core::strUtil::IsEqual(core::string_view(pObject->debugName), name)) {
            return pObject;
        }
    }

    return nullptr;
}

// ----------------------------------------------

void XSound::setPosition(SndObjectHandle object, const Transformf& trans)
{
    SoundObject* pSound = SoundHandleToObject(object);
    pSound->trans = trans;
    pSound->flags.Set(SoundFlag::Position);

    if (pSound->flags.IsSet(SoundFlag::Registered)) {
        AKRESULT res = AK::SoundEngine::SetPosition(object, TransToAkPos(trans));
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error setting position of object. %s", AkResult::ToString(res, desc));
        }
    }
}

void XSound::setPosition(SndObjectHandle* pObjects, const Transformf* pTrans, size_t num)
{
    for (size_t i = 0; i < num; i++) {
        SoundObject* pSound = SoundHandleToObject(pObjects[i]);
        pSound->trans = pTrans[i];
        pSound->flags.Set(SoundFlag::Position);

        if (!pSound->flags.IsSet(SoundFlag::Registered)) {
            continue;
        }

        AKRESULT res = AK::SoundEngine::SetPosition(pObjects[i], TransToAkPos(pTrans[i]));
        if (res != AK_Success) {
            AkResult::Description desc;
            X_ERROR("SoundSys", "Error setting position of object. %s", AkResult::ToString(res, desc));
        }
    }
}

// ----------------------------------------------

void XSound::stopAll(SndObjectHandle object)
{
    // we don't have todo anything for none global objects currently.
    // the active counts will get updated correctly.

    SoundEngine::StopAll(object);
}

PlayingID XSound::postEvent(EventID event, SndObjectHandle object)
{
    if (object != GLOBAL_OBJECT_ID) {
        SoundObject* pObject = SoundHandleToObject(object);
        if (!pObject->flags.IsSet(SoundFlag::Registered)) {
            registerObjectSndEngine(pObject);

            X_ASSERT(pObject->activeEvents == 0, "Unexpected active event count")(pObject->activeEvents);
        }

        ++pObject->activeEvents;
    }

    auto playingId = SoundEngine::PostEvent(event, object, AkCallbackType::AK_EndOfEvent, postEventCallback_s, this);
    if (playingId == AK_INVALID_PLAYING_ID) {
        X_ERROR("Sound", "Failed to post event %" PRIu32 " object: %" PRIu32, event, object);
    }
    else {
        // goaty
        X_LOG0("Sound", "PlayingID: %" PRIu32, playingId);
    }

    return playingId;
}

PlayingID XSound::postEvent(const char* pEventStr, SndObjectHandle object)
{
    return postEvent(AK::SoundEngine::GetIDFromString(pEventStr), object);
}

void XSound::postEventAtPosition(EventID event, const EventSwitches& switches, const Transformf& trans)
{
    // create a temp sound object for playing the sound at postion X.
    auto object = registerObject("PostAtPostTmp");
    setPosition(object, trans);

    SoundObject* pObject = SoundHandleToObject(object);
    pObject->flags.Set(SoundFlag::Temp);
    
    if (switches.ground != DEFAULT_SWITCH_STATE_ID) {
        setSwitch(AK::SWITCHES::MATERIAL::GROUP, switches.ground, object);
    }

    postEvent(event, object);
}

void XSound::stopPlyingID(PlayingID id)
{
    if (id != INVALID_PLAYING_ID) {
        AK::SoundEngine::StopPlayingID(id, 0, AkCurveInterpolation_Linear);
        return;
    }

    X_ERROR("Sound", "StopPlaying called with invalid playing ID");
}

PlayingID XSound::playVideoAudio(int32_t channels, int32_t sampleFreq, AudioBufferDelegate dataCallback, SndObjectHandle object)
{
    auto playingID = postEvent("VideoAudio_Start", object);
    
    core::CriticalSection::ScopedLock lock(memStreamCS_);

    // so need to be able to play video on objects, really i should support multiple streams per object.
    // so in order to stop the stream, i basically should just give you a id.
    MemoryInput mi;
    mi.playingID = playingID;
    mi.channels = channels;
    mi.sampleFreq = sampleFreq;
    mi.sndObj = object;
    mi.callback = dataCallback;

    memoryInputStreams_.push_back(mi);
    return playingID;
}

void XSound::stopVideoAudio(PlayingID id)
{
    SndObjectHandle object = INVALID_OBJECT_ID;

    {
        core::CriticalSection::ScopedLock lock(memStreamCS_);

        auto it = std::find_if(memoryInputStreams_.begin(), memoryInputStreams_.end(), [id](const MemoryInput& mi) {
            return mi.playingID == id;
        });

        if (it == memoryInputStreams_.end()) {
            X_ERROR("Sound", "Failed to find memory stream with playingID: %" PRIu32, id);
            return;
        }

        object = it->sndObj;
        memoryInputStreams_.remove(it);
    }

    // Tell the plugin to stop asking for data.
    SoundEngine::PostEvent("VideoAudio_Stop", object);
}

void XSound::setOcclusionType(SndObjectHandle object, OcclusionType::Enum type)
{
    X_ASSERT(object != INVALID_OBJECT_ID && object != GLOBAL_OBJECT_ID, "Invalid object handle for occlusion")(object);

    SoundObject* pSound = SoundHandleToObject(object);

    if (type == OcclusionType::None) {
        if (pSound->occType == OcclusionType::None) {
            return;
        }

        // remove.
        occlusion_.remove(pSound);
        pSound->flags.Remove(SoundFlag::Occlusion);
    }
    else {
        occlusion_.push_back(pSound);
        pSound->flags.Set(SoundFlag::Occlusion);
    }

    pSound->occType = type;
}

void XSound::setMaterial(SndObjectHandle object, engine::MaterialSurType::Enum surfaceType)
{
    AkSwitchStateID state;

    static_assert(engine::MaterialSurType::ENUM_COUNT == 26, "More surface types? this needs updating");

    switch (surfaceType) {
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

void XSound::setSwitch(SwitchGroupID group, SwitchStateID state, SndObjectHandle object)
{
    AKRESULT res = AK::SoundEngine::SetSwitch(group, state, object);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error when setting switch. group: %" PRIu32 " state: %" PRIu32 " %s", state, group, AkResult::ToString(res, desc));
    }
}

void XSound::setRTPCValue(RtpcID id, RtpcValue val, SndObjectHandle object,
    core::TimeVal changeDuration, CurveInterpolation::Enum fadeCurve)
{
    AKRESULT res = AK::SoundEngine::SetRTPCValue(id, val, object, ToAkTime(changeDuration), ToAkCurveInterpolation(fadeCurve));
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error set RTPC failed. %s", AkResult::ToString(res, desc));
    }
}

// ------------------ Packages ----------------------------

bool XSound::loadPackage(const char* pName)
{
    Package pck;
    pck.name = pName;

    auto loaded = std::find_if(packages_.begin(), packages_.end(), [&pck](const Package& p) -> bool {
        return pck.name == p.name;
    });

    if (loaded) {
        X_ERROR("SoundSys", "Package already loaded \"%s\"", pName);
        return true;
    }

    core::StackString<128, AkOSChar> akName(pck.name.begin(), pck.name.end());

    AKRESULT res = ioHook_.LoadFilePackage(akName.c_str(), pck.pckID);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Failed to load package \"%s\" %s", pName, AkResult::ToString(res, desc));
        return false;
    }

    packages_.emplace_back(std::move(pck));
    return true;
}

void XSound::unloadAllPackages(void)
{
    AKRESULT res = ioHook_.UnloadAllFilePackages();
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Failed to unload packages %s", AkResult::ToString(res, desc));
    }
}

// ------------------ Banks ----------------------------

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
        X_ERROR("SoundSys", "Failed to load bank \"%s\" %s", pName, AkResult::ToString(res, desc));
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

AkBankID XSound::getBankId(const char* pName) const
{
    core::Path<char> name(pName);
    name.toLower();
    name.removeExtension();

    return getIDFromStr(name.c_str());
}

XSound::Bank* XSound::getBankForID(AkBankID id)
{
    for (auto& bank : banks_) {
        if (bank.bankID == id) {
            return &bank;
        }
    }

    return nullptr;
}

bool XSound::waitForBankLoad(Bank& bank)
{
    while (bank.status == Bank::Status::Loading) {
        bankSignal_.wait();
    }

    return bank.status == Bank::Status::Loaded;
}

void XSound::listBanks(core::string_view searchPattern) const
{
    core::CriticalSection::ScopedLock lock(cs_);
    core::Array<const Bank*> sorted_banks(g_SoundArena);

    for (const auto& bank : banks_) {
        if (searchPattern.empty() || core::strUtil::WildCompare(searchPattern, core::string_view(bank.name))) {
            sorted_banks.push_back(&bank);
        }
    }

    std::sort(sorted_banks.begin(), sorted_banks.end(), [](const Bank* a, const Bank* b) {
        return a->name.compareInt(b->name) < 0;
    });

    X_LOG0("SoundSys", "------------ ^8Banks(%" PRIuS ")^7 ---------------", sorted_banks.size());

    for (const auto* pBank : sorted_banks) {
        X_LOG0("SoundSys", "^2%-32s^7 ID: ^20x%08" PRIx32 "^7 Status: ^2%s",
            pBank->name.c_str(), pBank->bankID, Bank::Status::ToString(pBank->status));
    }

    X_LOG0("SoundSys", "------------ ^8Banks End^7 --------------");
}

// ------------------ CallBacks Helpers ----------------------------

void XSound::postEventCallback_s(AkCallbackType eType, AkCallbackInfo* pCallbackInfo)
{
    reinterpret_cast<XSound*>(pCallbackInfo->pCookie)->postEventCallback(eType, pCallbackInfo);
}

void XSound::bankCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
    AkMemPoolId memPoolId, void* pCookie)
{
    reinterpret_cast<XSound*>(pCookie)->bankCallbackFunc(bankID, pInMemoryBankPtr, eLoadResult, memPoolId);
}

void XSound::bankUnloadCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
    AkMemPoolId memPoolId, void* pCookie)
{
    reinterpret_cast<XSound*>(pCookie)->bankUnloadCallbackFunc(bankID, pInMemoryBankPtr, eLoadResult, memPoolId);
}

void XSound::getFormatCallback_s(AkPlayingID in_playingID, AkAudioFormat& io_AudioFormat)
{
    static_cast<XSound*>(gEnv->pSound)->getFormatCallback(in_playingID, io_AudioFormat);
}

void XSound::executeCallback_s(AkPlayingID in_playingID, AkAudioBuffer* io_pBufferOut)
{
    static_cast<XSound*>(gEnv->pSound)->executeCallback(in_playingID, io_pBufferOut);
}

// ------------------ CallBacks ----------------------------

void XSound::postEventCallback(AkCallbackType eType, AkCallbackInfo* pCallbackInfo)
{
    if (pCallbackInfo->gameObjID == GLOBAL_OBJECT_ID) {
        return;
    }

    SoundObject* pObject = AKObjectToObject(pCallbackInfo->gameObjID);
    X_ASSERT_NOT_NULL(pObject);

    if (eType == AkCallbackType::AK_EndOfEvent) {
        auto* pEventInfo = static_cast<AkEventCallbackInfo*>(pCallbackInfo);

        --pObject->activeEvents;

        X_LOG0("SoundSys", "EndOfEvent: 0x%" PRIx32 " object %p debugName: %s", pEventInfo->eventID, pObject, pObject->debugName.c_str());

        if (pObject->flags.IsSet(SoundFlag::Temp)) {
            unRegisterObject(pCallbackInfo->gameObjID);
        }
    }
    else {
        X_ASSERT_UNREACHABLE();
    }
}

void XSound::bankCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
    AkMemPoolId memPoolId)
{
    X_UNUSED(pInMemoryBankPtr, memPoolId);
    Bank* pBank = X_ASSERT_NOT_NULL(getBankForID(bankID));

    if (eLoadResult == AK_Success) {
        pBank->status = Bank::Status::Loaded;
        X_LOG0("SoundSys", "bank \"%s\" loaded", pBank->name.c_str());
    }
    else {
        pBank->status = Bank::Status::Error;

        AkResult::Description desc;
        X_LOG0("SoundSys", "bank \"%s\" failed to load. %s", pBank->name.c_str(), AkResult::ToString(eLoadResult, desc));
    }

    bankSignal_.raise();
}

void XSound::bankUnloadCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult,
    AkMemPoolId memPoolId)
{
    X_UNUSED(pInMemoryBankPtr, memPoolId, eLoadResult);

    core::CriticalSection::ScopedLock lock(cs_);

    auto it = std::remove_if(banks_.begin(), banks_.end(), [bankID](const Bank& b) {
        return b.bankID == bankID;
    });

    banks_.erase(it, banks_.end());
}

void XSound::getFormatCallback(AkPlayingID in_playingID, AkAudioFormat& io_AudioFormat)
{
    X_UNUSED(in_playingID, io_AudioFormat);

    core::CriticalSection::ScopedLock lock(memStreamCS_);

    auto* pStream = memoryInputForID(in_playingID);
    if (!pStream) {
        X_ERROR("SoundSys", "No memory stream for playing id: %" PRIu32, in_playingID);
        return;
    }

    AkChannelConfig channelCfg;
    
    if (pStream->channels == 1)
    {
        channelCfg.uNumChannels = 1;
        channelCfg.uChannelMask = AK_SPEAKER_SETUP_MONO;
    }
    else if (pStream->channels == 2)
    {
        channelCfg.uNumChannels = 2;
        channelCfg.uChannelMask = AK_SPEAKER_SETUP_STEREO;
    }
    else
    {
        X_ASSERT_UNREACHABLE();
    }

    // TODO: allow customisation.
    io_AudioFormat.SetAll(
        pStream->sampleFreq,                  // Sample rate
        channelCfg,
        32,						// Bits per samples
        4,						// bytes per samples
        AK_FLOAT,			    // feeding floats
        AK_NONINTERLEAVED       // for floats the plugin only supports non :S ?
    );
}

void XSound::executeCallback(AkPlayingID in_playingID, AkAudioBuffer* io_pBufferOut)
{
    X_UNUSED(in_playingID, io_pBufferOut);

    core::CriticalSection::ScopedLock lock(memStreamCS_);

    auto* pStream = memoryInputForID(in_playingID);
    if (!pStream) {
        io_pBufferOut->uValidFrames = 0;
        io_pBufferOut->eState = AK_Fail;
        X_ERROR("SoundSys", "No memory stream for playing id: %" PRIu32, in_playingID);
        return;
    }

//    X_ASSERT(io_pBufferOut->NumChannels() == 2, "Channel count mismatch")();

    AudioBuffer ab(
        io_pBufferOut->GetChannel(0),
        io_pBufferOut->NumChannels(),
        io_pBufferOut->MaxFrames()
    );

    auto result = pStream->callback.Invoke(ab);

    switch (result)
    {
        case BufferResult::DataReady:
            io_pBufferOut->uValidFrames = safe_static_cast<AkUInt16>(ab.validFrames());
            io_pBufferOut->eState = AK_DataReady;
            break;
        case BufferResult::NoDataReady:
            io_pBufferOut->eState = AK_NoDataReady;
            X_ERROR("Sound", "Memory buffer starvation");
            break;
        case BufferResult::Paused:
            io_pBufferOut->eState = AK_NoDataReady;
            break;
        case BufferResult::NoMoreData:
            io_pBufferOut->eState = AK_NoMoreData;
            break;
    }
}


// ----------------------------------------------

XSound::MemoryInput* XSound::memoryInputForID(AkPlayingID playingID)
{
    auto it = std::find_if(memoryInputStreams_.begin(), memoryInputStreams_.end(), [playingID](const MemoryInput& ms) {
        return ms.playingID == playingID;
    });

    if (it == memoryInputStreams_.end()) {
        return nullptr;
    }

    return it;
}

// ------------------ CoreEvnt ----------------------------

void XSound::OnCoreEvent(const CoreEventData& ed)
{
    if (!AK::SoundEngine::IsInitialized()) {
        return;
    }

    if (ed.event == CoreEvent::CHANGE_FOCUS) 
    {
        auto focusLost = ed.focus.active == 0;

        if (vars_.SuspectOnFocusLost())
        {
            mute(focusLost);
        }
    }
}

// ------------------ Commands ----------------------------

// snd_set_rtpc <name> <state> <ObjectId>
void XSound::cmd_SetRtpc(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 3) {
        X_WARNING("Console", "snd_set_rtpc <name> <value> <ObjectId>");
        return;
    }

    auto nameStr = pArgs->GetArg(1);
    auto valueStr = pArgs->GetArg(2);

    // optional ObjectId
    AkGameObjectID objectId = GLOBAL_OBJECT_ID;
    if (pArgs->GetArgCount() > 3) {
        auto objectIDStr = pArgs->GetArg(1);
        objectId = core::strUtil::StringToInt<AkGameObjectID>(objectIDStr.begin(), objectIDStr.end());
    }

    auto nameID = getIDFromStr(nameStr);
    float value = core::strUtil::StringToFloat(valueStr.begin(), valueStr.end());

    AKRESULT res = SoundEngine::SetRTPCValue(nameID, value, objectId);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error setting RTPC value. %s", AkResult::ToString(res, desc));
    }
}

// snd_set_switchstate <name> <state> <ObjectId>
void XSound::cmd_SetSwitchState(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 3) {
        X_WARNING("Console", "snd_set_switchstate <name> <state> <ObjectId>");
        return;
    }

    auto nameStr = pArgs->GetArg(1);
    auto stateStr = pArgs->GetArg(2);

    // optional ObjectId
    AkGameObjectID objectId = GLOBAL_OBJECT_ID;
    if (pArgs->GetArgCount() > 3) {
        auto objectIDStr = pArgs->GetArg(1);
        objectId = core::strUtil::StringToInt<AkGameObjectID>(objectIDStr.begin(), objectIDStr.end());
    }

    auto nameID = getIDFromStr(nameStr);
    auto stateID = getIDFromStr(stateStr);

    AKRESULT res = SoundEngine::SetSwitch(nameID, stateID, objectId);
    if (res != AK_Success) {
        AkResult::Description desc;
        X_ERROR("SoundSys", "Error setting switch state. %s", AkResult::ToString(res, desc));
    }
}

// snd_post_event <eventName> <ObjectId>
void XSound::cmd_PostEvent(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 2) {
        X_WARNING("Console", "snd_post_event <eventName> <ObjectId>");
        return;
    }

    auto eventName = pArgs->GetArg(1);
    auto eventId = getIDFromStr(eventName);

    // optional ObjectId
    if (pArgs->GetArgCount() < 3) {
        X_LOG0("Sound", "snd_post_event(Global): id: %" PRIu32, eventId);
        postEvent(eventId, GLOBAL_OBJECT_ID);
        return;
    }

    // we want a specific object.
    auto objectStr = pArgs->GetArg(1);
    SoundObject* pObject = findObjectForNick(objectStr);
    if (!pObject) {
        X_WARNING("Console", "Failed to find sound object with id: \"%.*s\"", objectStr.length(), objectStr.data());
        return;
    }

    const auto& pos = pObject->trans.pos;
    X_LOG0("Sound", "snd_post_event: id: %" PRIu32 " object: %.*s object-pos: (%g,%g,%g)", eventId, objectStr.length(), objectStr.data(), pos.x, pos.y, pos.z);

    postEvent(eventId, SoundObjToObjHandle(pObject));
}

// snd_stop_event <eventName> <ObjectId>
void XSound::cmd_StopEvent(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 2) {
        X_WARNING("Console", "snd_stop_event <eventName> <ObjectId>");
        return;
    }

    auto eventName = pArgs->GetArg(1);
    //	auto eventId = SoundEngine::GetIDFromString(pEventName);

    // optional ObjectId
    AkGameObjectID objectId = GLOBAL_OBJECT_ID;
    if (pArgs->GetArgCount() > 2) {
        auto objectIDStr = pArgs->GetArg(1);
        objectId = core::strUtil::StringToInt<AkGameObjectID>(objectIDStr.begin(), objectIDStr.end());
    }

    // TODO
    X_UNUSED(eventName);
    X_UNUSED(objectId);
}

// snd_stop_all_event <ObjectId>
void XSound::cmd_StopAllEvent(core::IConsoleCmdArgs* pArgs)
{
    // optional ObjectId
    if (pArgs->GetArgCount() < 2) {
        X_LOG0("Sound", "snd_stop_all(Global)");
        stopAll(GLOBAL_OBJECT_ID);
        return;
    }

    // we want a specific object.
    auto objectStr = pArgs->GetArg(1);
    SoundObject* pObject = findObjectForNick(objectStr);

    if (!pObject) {
        X_WARNING("Console", "Failed to find sound object with id: \"%.*s\"", objectStr.length(), objectStr.data());
        return;
    }

    const auto& pos = pObject->trans.pos;
    X_LOG0("Sound", "snd_stop_all: object: %.*s object-pos: (%g,%g,%g)", objectStr.length(), objectStr.data(), pos.x, pos.y, pos.z);

    stopAll(SoundObjToObjHandle(pObject));
}

void XSound::cmd_ListBanks(core::IConsoleCmdArgs* pArgs)
{
    core::string_view searchPattern;

    if (pArgs->GetArgCount() >= 2) {
        searchPattern = pArgs->GetArg(1);
    }

    listBanks(searchPattern);
}

X_NAMESPACE_END
