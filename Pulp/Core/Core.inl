#pragma once



X_INLINE core::ITimer* XCore::GetITimer(void)
{
	return env_.pTimer;
}

X_INLINE input::IInput* XCore::GetIInput(void)
{
	return env_.pInput;
}

X_INLINE core::IConsole* XCore::GetIConsole(void)
{
	return env_.pConsole;
}

X_INLINE core::IFileSys* XCore::GetIFileSys(void)
{
	return env_.pFileSys;
}

X_INLINE sound::ISound* XCore::GetISound(void)
{
	return env_.pSound;
}

X_INLINE engine::I3DEngine* XCore::Get3DEngine(void)
{
	return env_.p3DEngine;
}

X_INLINE script::IScriptSys* XCore::GetISscriptSys(void)
{
	return env_.pScriptSys;
}

X_INLINE render::IRender* XCore::GetIRender(void)
{
	return env_.pRender;
}

X_INLINE font::IFontSys* XCore::GetIFontSys(void)
{
	return env_.pFontSys;
}

X_INLINE core::V2::JobSystem* XCore::GetJobSystem(void)
{
	return env_.pJobSys;
}

X_INLINE physics::IPhysics* XCore::GetPhysics(void)
{
	return env_.pPhysics;
}

X_INLINE core::profiler::IProfiler* XCore::GetProfiler(void)
{
#if X_ENABLE_PROFILER
	return pProfiler_;
#else
	return nullptr;
#endif // !X_ENABLE_PROFILER
}

X_INLINE core::IXDirectoryWatcher* XCore::GetDirWatcher(void)
{
	return &dirWatcher_;
}

X_INLINE core::IXHotReloadManager* XCore::GetHotReloadMan(void)
{
	return this;
}


X_INLINE ICoreEventDispatcher* XCore::GetCoreEventDispatcher(void)
{
	return pEventDispatcher_;
}

X_INLINE core::ILog* XCore::GetILog(void)
{
	return env_.pLog;
}

X_INLINE core::Crc32* XCore::GetCrc32(void)
{
	return pCrc32_;
}

X_INLINE core::CpuInfo* XCore::GetCPUInfo(void)
{
	return pCpuInfo_;
}


X_INLINE core::xWindow* XCore::GetGameWindow(void)
{
	return pWindow_;
}

X_INLINE core::AssetLoader* XCore::GetAssetLoader(void)
{
	return &assetLoader_;
}

X_INLINE SCoreGlobals* XCore::GetGlobalEnv(void)
{
	return &env_;
}

X_INLINE core::MallocFreeAllocator* XCore::GetGlobalMalloc(void)
{
	return &malloc_;
}
