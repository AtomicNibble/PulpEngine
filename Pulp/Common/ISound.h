#pragma once


#ifndef _X_SOUND_I_H_
#define _X_SOUND_I_H_

#include <Time\TimeVal.h>
#include <Hashing\Fnva1Hash.h>

#include <IMaterial.h>

X_NAMESPACE_BEGIN(sound)

typedef core::Hash::Fnv1Val HashVal;
typedef uintptr_t GameObjectID;
typedef HashVal EventID;
typedef HashVal RtpcID;
typedef HashVal SwitchGroupID;
typedef HashVal SwitchStateID;
typedef float RtpcValue;


static const GameObjectID GLOBAL_OBJECT_ID = static_cast<GameObjectID>(-2);
static const GameObjectID INVALID_OBJECT_ID = static_cast<GameObjectID>(-1);


X_DECLARE_ENUM(CurveInterpolation)(
	Log3,			///< Log3
	Sine,			///< Sine
	Log1,			///< Log1
	InvSCurve,		///< Inversed S Curve
	Linear,			///< Linear (Default)
	SCurve,			///< S Curve
	Exp1,			///< Exp1
	SineRecip,		///< Reciprocal of sine curve
//	Exp3,			///< Exp3 == is same value as LastFadeCurve -_-
	LastFadeCurve,	///< Update this value to reflect last curve available for fades
	Constant		/// constant ( not valid for fading values )
);


X_INLINE uint32_t GetIDFromStr(const char* pStr)
{
	X_ASSERT(core::strUtil::IsLower(pStr), "must be lower case")(pStr);
	return core::Hash::Fnv1Hash(pStr, std::strlen(pStr));
}

X_INLINE uint32_t GetIDFromStr(const char* pStr, size_t len)
{
	X_ASSERT(core::strUtil::IsLower(pStr, pStr + len), "must be lower case")(pStr);
	return core::Hash::Fnv1Hash(pStr, len);
}

namespace literals
{

	template<uint32_t N>
	constexpr X_INLINE uint32_t force_hash(void) {
		return N;
	}
	 
	X_INLINE constexpr uint32_t operator"" _soundId(const char* const pStr, const size_t strLen)
	{
		return core::Hash::Fnv1Const::Internal::Hash(pStr, strLen, core::Hash::Fnv1Const::default_offset_basis);
	}

}
	

struct ISound : public core::IEngineSysBase
{
	virtual ~ISound(){};

	virtual bool asyncInitFinalize(void) X_ABSTRACT;

	// ting tong wong, sing me a song in a thong!
	virtual void Update(void) X_ABSTRACT;

	// load banks, async.
	virtual void loadBank(const char* pName) X_ABSTRACT;
	virtual void unLoadBank(const char* pName) X_ABSTRACT;

	// Shut up!
	virtual void Mute(bool mute) X_ABSTRACT;

	virtual void SetListenPos(const Transformf& trans) X_ABSTRACT;


	// Volume
	virtual void SetMasterVolume(float vol) X_ABSTRACT;
	virtual void SetMusicVolume(float vol) X_ABSTRACT;
	virtual void SetVoiceVolume(float vol) X_ABSTRACT;
	virtual void SetSFXVolume(float vol) X_ABSTRACT;

	virtual uint32_t GetIDFromStr(const char* pStr) const X_ABSTRACT;
	virtual uint32_t GetIDFromStr(const wchar_t* pStr) const X_ABSTRACT;

	// the id is passed in, so could just pass pointer value in then use that as id.
	virtual bool RegisterObject(GameObjectID object, const char* pNick = nullptr) X_ABSTRACT;
	virtual bool UnRegisterObject(GameObjectID object) X_ABSTRACT;
	virtual void UnRegisterAll(void) X_ABSTRACT;

	virtual void SetPosition(GameObjectID object, const Transformf& trans) X_ABSTRACT;
	virtual void SetPosition(GameObjectID* pObjects, const Transformf* pTrans, size_t num) X_ABSTRACT;


	// INVALID_OBJECT_ID stops all sounds.
	virtual void StopAll(GameObjectID object = INVALID_OBJECT_ID) X_ABSTRACT;

	// events
	virtual void PostEvent(EventID event, GameObjectID object) X_ABSTRACT;

	virtual void SetMaterial(GameObjectID object, engine::MaterialSurType::Enum surfaceType) X_ABSTRACT;
	virtual void SetSwitch(SwitchGroupID group, SwitchStateID state, GameObjectID object) X_ABSTRACT;
	virtual void SetRTPCValue(RtpcID id, RtpcValue val, GameObjectID object = INVALID_OBJECT_ID,
		core::TimeVal changeDuration = core::TimeVal(0ll), CurveInterpolation::Enum fadeCurve = CurveInterpolation::Linear) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !_X_SOUND_I_H_
