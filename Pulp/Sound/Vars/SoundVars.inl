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

X_INLINE bool SoundVars::EnableCulling(void) const
{
	return enableCulling_ != 0;
}

X_INLINE int32_t SoundVars::EnableDebugRender(void) const
{
	return enableDebugRender_;
}

// ----------------------

X_INLINE float SoundVars::debugObjectScale(void) const
{
	return debugObjectScale_;
}

X_INLINE float SoundVars::debugTextSize(void) const
{
	return debugTextSize_;
}

X_INLINE float SoundVars::RegisteredCullDistance(void) const
{
	return registeredCullDistance_;
}

X_INLINE float SoundVars::OcclusionRefreshRate(void) const
{
	return occlusionRefreshRate_;
}

// ----------------------

X_INLINE uint32_t SoundVars::SoundEngineDefaultMemoryPoolBytes(void) const
{
	return soundEngineDefaultMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::SoundEngineLowerDefaultMemoryPoolBytes(void) const
{
	return soundEngineLowerDefaultPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::StreamManagerMemoryPoolBytes(void) const
{
	return streamManagerMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::StreamDeviceMemoryPoolBytes(void) const
{
	return streamDeviceMemoryPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::CommandQueueMemoryPoolBytes(void) const
{
	return commandQueueMemoryPoolSize_ << 10;
}

// ----------------------

X_INLINE uint32_t SoundVars::MonitorMemoryPoolBytes(void) const
{
	return monitorMemoryPoolSize_ << 10;
}

X_INLINE uint32_t SoundVars::MonitorQueueMemoryPoolBytes(void) const
{
	return monitorQueueMemoryPoolSize_ << 10;
}



X_NAMESPACE_END