#pragma once

#include <ICore.h>
#include <Platform\Module.h>

X_USING_NAMESPACE;

#include "Memory\MemoryTrackingPolicies\ExtendedMemoryTracking.h"
#include "Memory\MemoryTrackingPolicies\FullMemoryTracking.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::MultiThreadPolicy<core::Spinlock>,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
    >
    CoreArena;

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

    bool Init(HINSTANCE hInstance, const wchar_t* pInCmdLine);
    int MainLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void Error(core::string_view errorText);

    // Main Engine loop.
    bool PumpMessages();

private:
    AssetHandler assertCallback_;
    core::Module::Handle hSystemHandle_;
    ICore* pICore_;

    core::MallocFreeAllocator allocator_;
    CoreArena* pArena_;
};
