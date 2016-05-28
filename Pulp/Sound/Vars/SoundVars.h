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

	X_INLINE uint32_t SoundEngineDefaultMemoryPoolBytes(void);
	X_INLINE uint32_t SoundEngineLowerDefaultMemoryPoolBytes(void);

	X_INLINE uint32_t StreamManagerMemoryPoolBytes(void);
	X_INLINE uint32_t StreamDeviceMemoryPoolBytes(void);

	X_INLINE uint32_t CommandQueueMemoryPoolBytes(void);

	X_INLINE uint32_t MonitorMemoryPoolBytes(void);
	X_INLINE uint32_t MonitorQueueMemoryPoolBytes(void);

private:
	core::ICVar* var_vol_master_;
	core::ICVar* var_vol_music_;
	core::ICVar* var_vol_sfx_;
	core::ICVar* var_vol_voice_;

	int32_t enableCommSys_;
	int32_t enableOutputCapture_;

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
