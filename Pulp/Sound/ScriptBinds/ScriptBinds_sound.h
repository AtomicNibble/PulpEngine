#pragma once

#include <IScriptSys.h>

X_NAMESPACE_BEGIN(sound)

class XSound;

class ScriptBinds_Sound : public script::IScriptBindsBase
{
public:
	ScriptBinds_Sound(script::IScriptSys* pSS, XSound* pSound);
	~ScriptBinds_Sound();

	void bind(void);

private:
	int32_t PostEvent(script::IFunctionHandler* pH);
	int32_t SetSwitch(script::IFunctionHandler* pH);
	int32_t SetStages(script::IFunctionHandler* pH);
	int32_t SetParam(script::IFunctionHandler* pH);

	int32_t SetMasterVol(script::IFunctionHandler* pH);
	int32_t SetMusicVol(script::IFunctionHandler* pH);
	int32_t SetVoiceVol(script::IFunctionHandler* pH);
	int32_t SetSFXVol(script::IFunctionHandler* pH);

private:
	XSound* pSound_;
};

X_NAMESPACE_END