#pragma once

#include <ICore.h>

#define WIN_ENGINE_WINDOW_CLASSNAME "WinCatEngine"

X_USING_NAMESPACE;

#include "Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h"
#include "Memory\MemoryTrackingPolicies\FullMemoryTracking.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
	core::SimpleBoundsChecking,
#if X_SUPER || 1
	core::SimpleMemoryTracking,
	core::NoMemoryTagging
#else 
	core::FullMemoryTracking,
	core::SimpleMemoryTagging
#endif // !X_SUPER

> CoreArena;


class AssetHandler : public IAssertHandler
{
public:
	AssetHandler(void);
	virtual ~AssetHandler(void);

private:

	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;
};



class EngineApp
{
public:
	EngineApp();
	~EngineApp();

	bool Init(const wchar_t* sInCmdLine);
	int	MainLoop();

private:

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Main Engine loop.
	bool PumpMessages();
private:
	AssetHandler assertCallback_;
	HMODULE hSystemHandle_;
	ICore* pICore_;

	core::MallocFreeAllocator allocator_;
	CoreArena* pArena_; 
};

