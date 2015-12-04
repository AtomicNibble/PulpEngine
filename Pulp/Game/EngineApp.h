#pragma once

#include <ICore.h>

#define WIN_ENGINE_WINDOW_CLASSNAME "WinCatEngine"

X_USING_NAMESPACE;

#include "Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h"
#include "Memory\MemoryTrackingPolicies\FullMemoryTracking.h"

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
//	core::SimpleMemoryTracking,
		core::FullMemoryTracking,
	//	core::ExtendedMemoryTracking,
	core::SimpleMemoryTagging
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

