#include "stdafx.h"
#include "ScriptBinds_sound.h"

#include <ISound.h>


X_NAMESPACE_BEGIN(script)

using namespace sound;

#define X_SOUND_REG_FUNC(func)  \
{ \
	ScriptFunction Delegate; \
	Delegate.Bind<XBinds_Sound, &XBinds_Sound::func>(this); \
	RegisterFunction(#func, Delegate); \
}


XBinds_Sound::XBinds_Sound(IScriptSys* pScriptSystem, ICore* pCore)
{
	pCore_ = pCore;

	X_ASSERT_NOT_NULL(pCore_);

	XScriptableBase::Init(pScriptSystem, pCore);
	SetGlobalName("Sound");


	X_SOUND_REG_FUNC(Precache);
	X_SOUND_REG_FUNC(Play);
	X_SOUND_REG_FUNC(PlayEx);


	X_SOUND_REG_FUNC(IsPlaying);
	X_SOUND_REG_FUNC(SetSoundVolume);
	X_SOUND_REG_FUNC(GetSoundVolume);
	X_SOUND_REG_FUNC(SetSoundLoop);
	X_SOUND_REG_FUNC(SetSoundPaused);

	X_SOUND_REG_FUNC(StopSound);
	X_SOUND_REG_FUNC(SetSoundPosition);

}

XBinds_Sound::~XBinds_Sound()
{

}

ISound* XBinds_Sound::GetSoundPtr(IFunctionHandler* pH, int index)
{
	X_UNUSED(pH);
	X_UNUSED(index);
	X_ASSERT_NOT_IMPLEMENTED();
/*
	ScriptValueType::Enum type = pH->GetParamType(index);

	if (type == svtPointer) // this is a script handle
	{
		ScriptHandle soundID;
		if (!pH->GetParam(index, soundID))
			return 0;

		return m_pSoundSystem->GetSound((tSoundID)soundID.n);
	}
	else if (vType != svtNull)
	{
		SmartScriptTable tbl;
		if (!pH->GetParam(index, tbl))
			return 0;

		void * ptr = ((CScriptTable*)tbl.GetPtr())->GetUserDataValue();
		if (!ptr)
			return 0;

		return *static_cast<ISound**>(ptr);
	}
	*/

	return nullptr;
}


int XBinds_Sound::Precache(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_name>


	return pH->EndFunction();
}

int XBinds_Sound::Play(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_name>


	return pH->EndFunction();
}

int XBinds_Sound::PlayEx(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);

	return pH->EndFunction();
}

int XBinds_Sound::IsPlaying(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_id>

	return pH->EndFunction();
}

int XBinds_Sound::SetSoundVolume(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(2);
	// <sound_id> <vol>


	return pH->EndFunction();
}

int XBinds_Sound::GetSoundVolume(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_id>


	return pH->EndFunction();
}

int XBinds_Sound::SetSoundLoop(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_id> <loop>


	return pH->EndFunction();
}

int XBinds_Sound::SetSoundPaused(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(2);
	// <sound_id> <paused>

	return pH->EndFunction();
}

int XBinds_Sound::StopSound(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(1);
	// <sound_id> 


	return pH->EndFunction();
}

int XBinds_Sound::SetSoundPosition(IFunctionHandler* pH)
{
	SCRIPT_CHECK_PARAMETERS(2);
	// <sound_id> <pos>

	return pH->EndFunction();
}


X_NAMESPACE_END
