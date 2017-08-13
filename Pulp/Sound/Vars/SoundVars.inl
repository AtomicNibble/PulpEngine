#pragma once


X_NAMESPACE_BEGIN(sound)


X_INLINE bool SoundVars::EnableComs(void) const
{
	return enableCommSys_ != 0;
}

X_INLINE bool SoundVars::EnableOutputCapture(void) const
{
	return enableOutputCapture_ != 0;
}

X_INLINE bool SoundVars::EnableDebugRender(void) const
{
	return enableDebugRender_ != 0;
}

// ----------------------

X_INLINE uint32_t SoundVars::SoundEngineDefaultMemoryPoolBytes(void)
{
	return soundEngineDefaultMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::SoundEngineLowerDefaultMemoryPoolBytes(void)
{
	return soundEngineLowerDefaultPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::StreamManagerMemoryPoolBytes(void)
{
	return streamManagerMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::StreamDeviceMemoryPoolBytes(void)
{
	return streamDeviceMemoryPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::CommandQueueMemoryPoolBytes(void)
{
	return commandQueueMemoryPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::MonitorMemoryPoolBytes(void)
{
	return monitorMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::MonitorQueueMemoryPoolBytes(void)
{
	return monitorQueueMemoryPoolSize_ << 10;
}



X_NAMESPACE_END