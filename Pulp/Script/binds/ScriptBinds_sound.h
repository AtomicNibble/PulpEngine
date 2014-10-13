#pragma once

#ifndef X_SCRIPT_BINDS_SOUND_H_
#define X_SCRIPT_BINDS_SOUND_H_

#include "ScriptableBase.h"


X_NAMESPACE_DECLARE(sound,
struct ISound;
)

X_NAMESPACE_BEGIN(script)

class XBinds_Sound : public XScriptableBase, public IScriptableBase
{
public:
	XBinds_Sound(IScriptSys* pScriptSystem, ICore* pCore);
	~XBinds_Sound() X_OVERRIDE;


	sound::ISound* GetSoundPtr(IFunctionHandler* pH, int index);

	int Precache(IFunctionHandler* pH);
	int Play(IFunctionHandler* pH);
	int PlayEx(IFunctionHandler* pH);

	int IsPlaying(IFunctionHandler* pH);
	int SetSoundVolume(IFunctionHandler* pH);
	int GetSoundVolume(IFunctionHandler* pH);
	int SetSoundLoop(IFunctionHandler* pH);
	int SetSoundPaused(IFunctionHandler* pH);

	int StopSound(IFunctionHandler* pH);
	int SetSoundPosition(IFunctionHandler* pH);

private:
	ICore* pCore_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_SOUND_H_