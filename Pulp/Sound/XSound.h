#pragma once

#ifndef _X_SOUNG_I_H_
#define _X_SOUNG_I_H_

#include "IO\AkIoHook.h"
#include "IO\AkFilePackageLowLevelIO.h"

#include "Vars\SoundVars.h"
#include "Util\Allocators.h"

#include <Threading\Signal.h>
#include <Assets\AssertContainer.h>
#include <Util\UniquePointer.h>

#include <ICore.h>

X_NAMESPACE_DECLARE(core,
                    struct ICVar;
                    struct IConsoleCmdArgs;
                    struct FrameData;
                    struct FrameView;)

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_DECLARE(physics,
                    struct IScene);

X_NAMESPACE_BEGIN(sound)

class ScriptBinds_Sound;

X_DECLARE_FLAGS8(SoundFlag)
(
    Registered,
    Position,
    Occlusion,
    Occluded);

typedef Flags<SoundFlag> SoundFlags;

struct SoundObject
{
    SoundObject()
    {
        occType = OcclusionType::None;
        activeEvents = 0;
    }

    Transformf trans;

    SoundFlags flags;
    OcclusionType::Enum occType;
    int16_t activeEvents;

//	core::TimeVal lastEvent;
#if X_SOUND_ENABLE_DEBUG_NAMES
    core::string debugName;
#endif
};

class XSound : public ISound
    , public ICoreEventListener
{
    struct Bank
    {
        X_DECLARE_ENUM(Status)
        (
            Loading,
            Loaded,
            Unloading,
            Error);

        Status::Enum status;
        AkBankID bankID;
        core::string name;
    };

    struct Package
    {
        AkUInt32 pckID;
        core::string name;
    };

    template<typename T>
    using ArrayMultiply = core::Array<T, core::ArrayAllocator<T>, core::growStrat::Multiply>;

    typedef core::Array<Bank> BanksArr;
    typedef core::Array<Package> PackageArr;

    typedef ArrayMultiply<SoundObject> SoundObjectArr;
    typedef ArrayMultiply<SoundObject*> SoundObjectPtrArr;

    typedef core::AssetPool<
        SoundObject,
        MAX_SOUND_OBJECTS,
        core::SingleThreadPolicy>
        SoundObjectPool;

public:
    XSound(core::MemoryArenaBase* arena);
    ~XSound() X_OVERRIDE;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;
    void registerScriptBinds(void);

    bool init(void) X_FINAL;
    bool asyncInitFinalize(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    void update(core::FrameData& frame) X_FINAL;
    void setPhysicsScene(physics::IScene* pScene) X_FINAL;

    // Shut up!
    void mute(bool mute) X_FINAL;

    void setListenPos(const Transformf& trans) X_FINAL;

    // Volume
    void setMasterVolume(float vol) X_FINAL;
    void setMusicVolume(float vol) X_FINAL;
    void setVoiceVolume(float vol) X_FINAL;
    void setSFXVolume(float vol) X_FINAL;

    uint32_t getIDFromStr(const char* pStr) const X_FINAL;
    uint32_t getIDFromStr(const wchar_t* pStr) const X_FINAL;

    SndObjectHandle registerObject(X_SOUND_DEBUG_NAME(const char* pNick)) X_FINAL;
    SndObjectHandle registerObject(const Transformf& trans X_SOUND_DEBUG_NAME_COM(const char* pNick)) X_FINAL;
    bool unRegisterObject(SndObjectHandle object) X_FINAL;
    void unRegisterAll(void) X_FINAL;

    void setPosition(SndObjectHandle object, const Transformf& trans) X_FINAL;
    void setPosition(SndObjectHandle* pObjects, const Transformf* pTrans, size_t num) X_FINAL;

    void stopAll(SndObjectHandle object) X_FINAL;

    void postEvent(EventID event, SndObjectHandle object) X_FINAL;
    void postEvent(const char* pEventStr, SndObjectHandle object) X_FINAL;

    void setOcclusionType(SndObjectHandle object, OcclusionType::Enum type) X_FINAL;
    void setMaterial(SndObjectHandle object, engine::MaterialSurType::Enum surfaceType) X_FINAL;
    void setSwitch(SwitchGroupID group, SwitchStateID state, SndObjectHandle object) X_FINAL;
    void setRTPCValue(RtpcID id, RtpcValue val, SndObjectHandle object = INVALID_OBJECT_ID,
        core::TimeVal changeDuration = core::TimeVal(0ll),
        CurveInterpolation::Enum fadeCurve = CurveInterpolation::Linear) X_FINAL;

    void loadPackage(const char* pName);

    void loadBank(const char* pName) X_FINAL;
    void unLoadBank(const char* pName) X_FINAL;
    AkBankID getBankId(const char* pName) const;
    Bank* getBankForID(AkBankID id);
    bool waitForBankLoad(Bank& bank);

    void listBanks(const char* pSearchString) const;

private:
    void drawDebug(void) const;
    void cullObjects(void);
    void performOcclusionChecks(void);

    void registerObjectSndEngine(SoundObject* pObject);
    void unregisterObjectSndEngine(SoundObject* pObject);

private:
    void freeDangling(void);
    SoundObject* allocObject(void);
    void freeObject(SoundObject* pObject);

    SoundObject* findObjectForNick(const char* pNick);

private:
    static void postEventCallback_s(AkCallbackType eType, AkCallbackInfo* pCallbackInfo);
    static void bankCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId, void* pCookie);
    static void bankUnloadCallbackFunc_s(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId, void* pCookie);

    void postEventCallback(AkCallbackType eType, AkCallbackInfo* pCallbackInfo);
    void bankCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId);
    void bankUnloadCallbackFunc(AkUInt32 bankID, const void* pInMemoryBankPtr, AKRESULT eLoadResult, AkMemPoolId memPoolId);

private:
    void OnCoreEvent(const CoreEventData& ed) X_FINAL;

private:
    void cmd_SetRtpc(core::IConsoleCmdArgs* pArgs);
    void cmd_SetSwitchState(core::IConsoleCmdArgs* pArgs);
    void cmd_PostEvent(core::IConsoleCmdArgs* pArgs);
    void cmd_StopEvent(core::IConsoleCmdArgs* pArgs);
    void cmd_StopAllEvent(core::IConsoleCmdArgs* pArgs);
    void cmd_ListBanks(core::IConsoleCmdArgs* pArgs);

private:
    core::MemoryArenaBase* arena_;
    engine::IPrimativeContext* pPrimCon_;
    physics::IScene* pScene_;

    AllocatorHooks allocators_;
    CAkFilePackageLowLevelIO<IOhook> ioHook_;

private:
    SoundVars vars_;
    core::TimeVal lastOcclusionUpdate_;

    mutable core::CriticalSection cs_;

    BanksArr banks_;
    PackageArr packages_;
    SoundObjectPool objectPool_;
    SoundObjectPtrArr objects_;
    SoundObjectPtrArr culledObjects_;
    SoundObjectPtrArr occlusion_;

    bool comsSysInit_;
    bool outputCaptureEnabled_;
    bool suspended_;
    bool _pad[1];

    Transformf listenerTrans_;

    core::Signal bankSignal_;

    ScriptBinds_Sound* pScriptBinds_;
};

X_NAMESPACE_END

#endif // !_X_SOUNG_I_H_
