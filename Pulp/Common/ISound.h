#pragma once

#ifndef _X_SOUND_I_H_
#define _X_SOUND_I_H_

#include <Time\TimeVal.h>
#include <Hashing\Fnva1Hash.h>

#include <IMaterial.h>

X_NAMESPACE_DECLARE(physics,
    struct IScene);

X_NAMESPACE_BEGIN(sound)

#define X_SOUND_ENABLE_DEBUG_NAMES 1

#if X_SOUND_ENABLE_DEBUG_NAMES
#define X_SOUND_DEBUG_NAME(expr) expr
#define X_SOUND_DEBUG_NAME_COM(expr) , expr
#define X_SOUND_DEBUG_NAME_PARAM(expr) , expr
#else
#define X_SOUND_DEBUG_NAME(expr)
#define X_SOUND_DEBUG_NAME_COM(expr)
#define X_SOUND_DEBUG_NAME_PARAM(expr)
#endif // !X_SOUND_ENABLE_DEBUG_NAMES

typedef core::Hash::Fnv1Val HashVal;
typedef uint64_t SndObjectHandle;
typedef uint32_t PlayingID;
typedef HashVal EventID;
typedef HashVal RtpcID;
typedef HashVal SwitchGroupID;
typedef HashVal SwitchStateID;
typedef float RtpcValue;

static const uint32_t MAX_SOUND_OBJECTS = 1 << 9;

static const SndObjectHandle GLOBAL_OBJECT_ID = static_cast<SndObjectHandle>(2);
static const SndObjectHandle LISTNER_OBJECT_ID = static_cast<SndObjectHandle>(1);
static const SndObjectHandle INVALID_OBJECT_ID = static_cast<SndObjectHandle>(-1);

struct AudioBuffer
{
    typedef float SamepleType;

    AudioBuffer(SamepleType* pBuffer, int32_t numChannels, int32_t maxFrames) :
        pBuffer_(pBuffer),
        numChannels_(numChannels),
        maxFrames_(maxFrames),
        validFrames_(0)
    {
    }

    X_INLINE SamepleType* getChannel(int32_t channelIdx) const {
        X_ASSERT(channelIdx < numChannels_, "Index out of range")(channelIdx, numChannels_);
        return pBuffer_ + (channelIdx * maxFrames_);
    }

    X_INLINE int32_t numChannels(void) const {
        return numChannels_;
    }

    X_INLINE int32_t maxFrames(void) const {
        return maxFrames_;
    }

    X_INLINE int32_t validFrames(void) const {
        return validFrames_;
    }

    X_INLINE void setValidFrames(int32_t num) {
        validFrames_ = num;
    }

    X_INLINE void zeroPadToMaxFrames(void) {
        // Zero out all channels.
        const auto numZeroFrames = maxFrames_ - validFrames_;
        if (numZeroFrames)
        {
            for (int32_t i = 0; i < numChannels_; ++i) {
                std::memset(getChannel(i) + validFrames_, 0, numZeroFrames * sizeof(SamepleType));
            }
            validFrames_ = maxFrames_;
        }
    }

private:
    SamepleType* pBuffer_;

    const int32_t numChannels_;
    const int32_t maxFrames_;
    int32_t validFrames_;
};

typedef core::Delegate<void(AudioBuffer& ab)> AudioBufferDelegate;

X_DECLARE_ENUM(CurveInterpolation)
(
    Log3,          ///< Log3
    Sine,          ///< Sine
    Log1,          ///< Log1
    InvSCurve,     ///< Inversed S Curve
    Linear,        ///< Linear (Default)
    SCurve,        ///< S Curve
    Exp1,          ///< Exp1
    SineRecip,     ///< Reciprocal of sine curve
                   //	Exp3,			///< Exp3 == is same value as LastFadeCurve -_-
    LastFadeCurve, ///< Update this value to reflect last curve available for fades
    Constant       /// constant ( not valid for fading values )
);

X_DECLARE_ENUM8(OcclusionType)
(
    None,
    SingleRay,
    MultiRay);

X_INLINE uint32_t getIDFromStr(const char* pStr)
{
    X_ASSERT(core::strUtil::IsLower(pStr), "must be lower case")(pStr);
    return core::Hash::Fnv1Hash(pStr, std::strlen(pStr));
}

X_INLINE uint32_t getIDFromStr(const char* pStr, size_t len)
{
    X_ASSERT(core::strUtil::IsLower(pStr, pStr + len), "must be lower case")(pStr);
    return core::Hash::Fnv1Hash(pStr, len);
}

namespace Literals
{
    template<uint32_t N>
    constexpr X_INLINE uint32_t force_hash(void)
    {
        return N;
    }

    X_INLINE constexpr uint32_t operator"" _soundId(const char* const pStr, const size_t strLen)
    {
        return core::Hash::Fnv1Const::Hash(pStr, strLen);
    }

} // namespace Literals

struct ISound : public core::IEngineSysBase
{
    virtual ~ISound() = default;

    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    // ting tong wong, sing me a song in a thong!
    virtual void update(core::FrameData& frame) X_ABSTRACT;
    virtual void setPhysicsScene(physics::IScene* pScene) X_ABSTRACT;

    // load banks, async.
    virtual void loadBank(const char* pName) X_ABSTRACT;
    virtual void unLoadBank(const char* pName) X_ABSTRACT;

    // Shut up!
    virtual void mute(bool mute) X_ABSTRACT;

    virtual void setListenPos(const Transformf& trans) X_ABSTRACT;

    // Volume
    virtual void setMasterVolume(float vol) X_ABSTRACT;
    virtual void setMusicVolume(float vol) X_ABSTRACT;
    virtual void setVoiceVolume(float vol) X_ABSTRACT;
    virtual void setSFXVolume(float vol) X_ABSTRACT;

    virtual uint32_t getIDFromStr(const char* pStr) const X_ABSTRACT;
    virtual uint32_t getIDFromStr(const wchar_t* pStr) const X_ABSTRACT;

    // the id is passed in, so could just pass pointer value in then use that as id.
    virtual SndObjectHandle registerObject(X_SOUND_DEBUG_NAME(const char* pNick = nullptr)) X_ABSTRACT;
    virtual SndObjectHandle registerObject(const Transformf& trans X_SOUND_DEBUG_NAME_COM(const char* pNick = nullptr)) X_ABSTRACT;
    virtual bool unRegisterObject(SndObjectHandle object) X_ABSTRACT;
    virtual void unRegisterAll(void) X_ABSTRACT;

    virtual void setPosition(SndObjectHandle object, const Transformf& trans) X_ABSTRACT;
    virtual void setPosition(SndObjectHandle* pObjects, const Transformf* pTrans, size_t num) X_ABSTRACT;

    // INVALID_OBJECT_ID stops all sounds.
    virtual void stopAll(SndObjectHandle object = INVALID_OBJECT_ID) X_ABSTRACT;

    // events
    virtual PlayingID postEvent(EventID event, SndObjectHandle object) X_ABSTRACT;
    virtual PlayingID postEvent(const char* pEventStr, SndObjectHandle object) X_ABSTRACT;

    virtual PlayingID playVideoAudio(int32_t channels, int32_t sampleFreq, 
        AudioBufferDelegate dataCallback, SndObjectHandle object) X_ABSTRACT;
    virtual void stopVideoAudio(PlayingID id) X_ABSTRACT;

    virtual void setOcclusionType(SndObjectHandle object, OcclusionType::Enum type) X_ABSTRACT;
    virtual void setMaterial(SndObjectHandle object, engine::MaterialSurType::Enum surfaceType) X_ABSTRACT;
    virtual void setSwitch(SwitchGroupID group, SwitchStateID state, SndObjectHandle object) X_ABSTRACT;
    virtual void setRTPCValue(RtpcID id, RtpcValue val, SndObjectHandle object = INVALID_OBJECT_ID,
        core::TimeVal changeDuration = core::TimeVal(0ll), CurveInterpolation::Enum fadeCurve = CurveInterpolation::Linear) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !_X_SOUND_I_H_
