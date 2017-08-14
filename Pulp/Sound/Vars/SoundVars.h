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

	void setMasterVolume(float v);
	void setMusicVolume(float vol);
	void setVoiceVolume(float vol);
	void setSFXVolume(float vol);

	X_INLINE bool EnableComs(void) const;
	X_INLINE bool EnableOutputCapture(void) const;
	X_INLINE bool EnableDebugRender(void) const;

	X_INLINE float debugObjectScale(void) const;

	X_INLINE float registeredCullDistance(void);

	X_INLINE uint32_t SoundEngineDefaultMemoryPoolBytes(void);
	X_INLINE uint32_t SoundEngineLowerDefaultMemoryPoolBytes(void);

	X_INLINE uint32_t StreamManagerMemoryPoolBytes(void);
	X_INLINE uint32_t StreamDeviceMemoryPoolBytes(void);

	X_INLINE uint32_t CommandQueueMemoryPoolBytes(void);

	X_INLINE uint32_t MonitorMemoryPoolBytes(void);
	X_INLINE uint32_t MonitorQueueMemoryPoolBytes(void);

private:
	core::ICVar* pVarVolMaster_;
	core::ICVar* pVarVolMusic_;
	core::ICVar* pVarVolSfx_;
	core::ICVar* pVarVolVoice_;

	int32_t enableCommSys_;
	int32_t enableOutputCapture_;

	int32_t enableDebugRender_;
	float debugObjectScale_;

	float registeredCullDistance_;

private:
	int32_t soundEngineDefaultMemoryPoolSize_;
	int32_t soundEngineLowerDefaultPoolSize_;

	int32_t streamManagerMemoryPoolSize_;
	int32_t streamDeviceMemoryPoolSize_;

	int32_t commandQueueMemoryPoolSize_;

	int32_t monitorMemoryPoolSize_;
	int32_t monitorQueueMemoryPoolSize_;
};


X_NAMESPACE_END

#include "SoundVars.inl"

#endif // !_X_SOUND_VARS_H_
