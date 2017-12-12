#pragma once

#ifndef X_SCRIPT_BINDS_SOUND_H_
#define X_SCRIPT_BINDS_SOUND_H_


X_NAMESPACE_DECLARE(sound,
struct ISound;
)

X_NAMESPACE_BEGIN(script)

class XBinds_Sound : public XScriptableBase
{
public:
	XBinds_Sound();
	~XBinds_Sound() X_OVERRIDE;

	void init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0) X_OVERRIDE;

	int32_t PostEvent(IFunctionHandler* pH);
	int32_t SetSwitch(IFunctionHandler* pH);
	int32_t SetStages(IFunctionHandler* pH);
	int32_t SetParam(IFunctionHandler* pH);

	int32_t SetMasterVol(IFunctionHandler* pH);
	int32_t SetMusicVol(IFunctionHandler* pH);
	int32_t SetVoiceVol(IFunctionHandler* pH);
	int32_t SetSFXVol(IFunctionHandler* pH);

private:
	sound::ISound* pSoundSys_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_SOUND_H_