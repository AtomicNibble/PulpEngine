#pragma once

#ifndef _X_SOUND_VARS_H_
#define _X_SOUND_VARS_H_

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(sound)

class SoundVars
{
public:
	SoundVars();
	~SoundVars();


	void RegisterVars(void);

	X_INLINE bool EnableComs(void) const;
	X_INLINE bool EnableOutputCapture(void) const;

private:
	core::ICVar* var_vol_master_;
	core::ICVar* var_vol_music_;
	core::ICVar* var_vol_sfx_;
	core::ICVar* var_vol_voice_;

	int32_t enableCommSys_;
	int32_t enableOutputCapture_;



};


X_INLINE bool SoundVars::EnableComs(void) const
{
	return enableCommSys_ != 0;
}

X_INLINE bool SoundVars::EnableOutputCapture(void) const
{
	return enableOutputCapture_ != 0;
}



X_NAMESPACE_END

#endif // !_X_SOUND_VARS_H_
